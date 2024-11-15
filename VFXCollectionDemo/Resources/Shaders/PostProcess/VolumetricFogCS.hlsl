#include "../Lighting/PBRLighting.hlsli"

static const uint NUM_THREADS = 64;
static const uint NUM_VIEW_STEPS = 8;
static const uint NUM_LIGHT_STEPS = 1;

static const float VIEW_STEP_SIZE = 1.2f;
static const float3 FOG_COLOR = float3(0.4f, 0.15f, 0.05f);

cbuffer LightConstantBuffer : register(b0)
{
#ifdef AREA_LIGHT
	AreaLight areaLight;
	AmbientLight ambientLight;
#else
	DirectionalLight directionalLight;
#endif
};

cbuffer MutableConstants : register(b1)
{
	float4x4 invViewProjection;
	
	uint widthU;
	uint area;
	float width;
	float height;

	float3 cameraDirection;
	float fogTiling;

	float3 cameraPosition;
	float distanceFalloffStart;

	float3 fogOffset;
	float distanceFalloffLength;
};

struct Input
{
	uint3 dispatchThreadId : SV_DispatchThreadID;
	uint3 groupId : SV_GroupID;
	uint groupIndex : SV_GroupIndex;
};

Texture3D<float4> fogMap : register(t0);
Texture3D<float4> turbulenceMap : register(t1);
Texture2D<float> sceneDepth : register(t2);
Texture2D<float4> sceneColor : register(t3);

#if (PARTICLE_LIGHT_SOURCE_NUMBER > 0)
StructuredBuffer<PointLight> particleLightBuffer : register(t4);
#endif

RWTexture2D<float4> sceneTarget : register(u0);

SamplerState samplerLinear : register(s0);

float3 ReconstructWorldPosition(float2 texCoord, float depth, float4x4 unprojectionMatrix)
{
	float4 screenPosition = float4(texCoord, depth, 1.0f);
	screenPosition.xy *= 2.0f.xx;
	screenPosition.xy -= 1.0f.xx;
	screenPosition.y = -screenPosition.y;
	
	float4 worldPosition = mul(unprojectionMatrix, screenPosition);
	
	return worldPosition.xyz / worldPosition.w;
}

float CalculateVisibilityFalloff(float sceneDistance)
{
	float result = saturate((sceneDistance - distanceFalloffStart) / distanceFalloffLength);
	result = result * result * (3.0f - 2.0f * result);
	
	return result;
}

#if defined(FSR)
float4 ProcessLightSources(float3 sceneDiffuse, float3 lightColor, float3 screenPoint, float3 worldPosition,
	float3 lightPosition, float3 viewStep, float3 viewDirection)
#else
float3 ProcessLightSources(float3 sceneDiffuse, float3 lightColor, float3 screenPoint, float3 worldPosition,
	float3 lightPosition, float3 viewStep, float3 viewDirection)
#endif
{
#if defined(FSR)
	float4 resultColor = float4(sceneDiffuse, 1.0f);
#else
	float3 resultColor = sceneDiffuse;
#endif
	
	float3 vWayPoint = screenPoint;
	
	[unroll]
	for (int vStepIndex = 0u; vStepIndex < NUM_VIEW_STEPS; vStepIndex++)
	{
		vWayPoint += viewStep;
		
		float3 distortionCoord = vWayPoint * fogTiling;
		float3 fogMapDistortion = turbulenceMap.SampleLevel(samplerLinear, distortionCoord, 0.0f).xyz;
		fogMapDistortion = fogMapDistortion * 2.0f - 1.0f.xxx;
		fogMapDistortion *= 0.063f;
		
		float fogAlpha = fogMap.SampleLevel(samplerLinear, vWayPoint * fogTiling + fogMapDistortion + fogOffset, 0.0f).w;
		
		float3 worldToWayPoint = vWayPoint - worldPosition;
		
		float occlusion = dot(worldToWayPoint, viewDirection) > 0.0f ? 0.0f : 1.0f;
		occlusion *= CalculateVisibilityFalloff(length(worldToWayPoint));
		occlusion *= lerp(0.0f, 0.8f, CalculateVisibilityFalloff(distance(worldPosition, float3(0.0f, 0.0f, 1.25f))));
		
		float3 absorptionCoeff = lerp(AIR_WET_ABSORPTION_COEFF, WATER_ABSORPTION_COEFF, fogAlpha);
		absorptionCoeff *= occlusion;
		
#if defined(FSR)
		resultColor = BouguerLambertBeerLaw(resultColor, VIEW_STEP_SIZE, absorptionCoeff.xyzz);
#else
		resultColor = BouguerLambertBeerLaw(resultColor, VIEW_STEP_SIZE, absorptionCoeff);
#endif
		
		float3 lightDir = vWayPoint - lightPosition;
		float lightStepLength = length(lightDir) / NUM_LIGHT_STEPS;
		float3 lightStep = lightDir / (NUM_LIGHT_STEPS + 1);
		lightDir = normalize(lightDir + 0.0000001f.xxx);
		
		float3 lWayPoint = vWayPoint;
		
		float3 scatteredLight = lightColor;
		
		[unroll]
		for (uint lStepIndex = 0u; lStepIndex < NUM_LIGHT_STEPS; lStepIndex++)
		{
			lWayPoint -= lightStep;
			
			distortionCoord = lWayPoint * fogTiling;
			fogMapDistortion = turbulenceMap.SampleLevel(samplerLinear, distortionCoord, 0.0f).xyz;
			fogMapDistortion = fogMapDistortion * 2.0f - 1.0f.xxx;
			fogMapDistortion *= 0.063f;
			
			fogAlpha = fogMap.SampleLevel(samplerLinear, lWayPoint * fogTiling + fogMapDistortion + fogOffset, 0.0f).w;
			
			absorptionCoeff = lerp(AIR_WET_ABSORPTION_COEFF, WATER_ABSORPTION_COEFF, fogAlpha);
			
			scatteredLight = BouguerLambertBeerLaw(scatteredLight, lightStepLength, absorptionCoeff);
		}
		
		float cosAngle = dot(viewDirection, -lightDir);
		float phaseValue = DoubleHenyeyGreensteinPhase(cosAngle);
		
		scatteredLight *= 4.0f * PI * SCATTERING_CONST * saturate(phaseValue) * occlusion / NUM_VIEW_STEPS;
		scatteredLight *= INV_WAVELENGTH_4;
		
#if defined(FSR)
		resultColor.xyz += scatteredLight;
		resultColor.w = max(resultColor.w, max(scatteredLight.x, max(scatteredLight.y, scatteredLight.z)));
#else
		resultColor += scatteredLight;
#endif
		
#if (PARTICLE_LIGHT_SOURCE_NUMBER > 0)
		[unroll]
		for (uint lightSourceIndex = 0u; lightSourceIndex < PARTICLE_LIGHT_SOURCE_NUMBER; lightSourceIndex++)
		{
			float3 particlePosition = particleLightBuffer[lightSourceIndex].position;
			lightDir = vWayPoint - particlePosition;
			lightStep = lightDir / 2.0f;
			lightStepLength = length(lightDir);
			lightDir = lightDir / (lightStepLength + 0.0000001f);
			
			distortionCoord = particlePosition * fogTiling;
			fogMapDistortion = turbulenceMap.SampleLevel(samplerLinear, distortionCoord, 0.0f).xyz;
			fogMapDistortion = fogMapDistortion * 2.0f - 1.0f.xxx;
			fogMapDistortion *= 0.063f;
			
			fogAlpha = fogMap.SampleLevel(samplerLinear, particlePosition * fogTiling + fogMapDistortion + fogOffset, 0.0f).w;
			
			absorptionCoeff = lerp(AIR_WET_ABSORPTION_COEFF, WATER_ABSORPTION_COEFF, fogAlpha);
			
			scatteredLight = BouguerLambertBeerLaw(particleLightBuffer[lightSourceIndex].color, lightStepLength, absorptionCoeff);
			
			cosAngle = dot(viewDirection, -lightDir);
			phaseValue = DoubleHenyeyGreensteinPhase(cosAngle);
			
			scatteredLight *= 4.0f * PI * SCATTERING_CONST * saturate(phaseValue) * occlusion / NUM_VIEW_STEPS;
			scatteredLight *= INV_WAVELENGTH_4;
			
#if defined(FSR)
			resultColor.xyz += scatteredLight;
			resultColor.w = max(resultColor.w, max(scatteredLight.x, max(scatteredLight.y, scatteredLight.z)));
#else
			resultColor += scatteredLight;
#endif
		}
#endif
	}
	
	return resultColor;
}

[numthreads(NUM_THREADS, 1, 1)]
void main(Input input)
{
	uint bufferIndex = input.dispatchThreadId.x;
	
	if (bufferIndex >= area)
		return;
	
	uint2 targetCoord = uint2(bufferIndex % widthU, bufferIndex / widthU);
	
	float2 texCoord;
	texCoord.x = targetCoord.x / width;
	texCoord.y = targetCoord.y / height;
	
	float4 sceneDiffuse = sceneColor.SampleLevel(samplerLinear, texCoord, 0.0f);
	
	float depth = sceneDepth.SampleLevel(samplerLinear, texCoord, 0.0f).x;
	float3 worldPosition = ReconstructWorldPosition(texCoord, depth, invViewProjection);
	float3 screenWorldPosition = ReconstructWorldPosition(texCoord, 0.0f, invViewProjection);
	
	float3 viewDirection = normalize(worldPosition - cameraPosition + 0.00001f.xxx);
	float3 viewStep = viewDirection * VIEW_STEP_SIZE;
	
#if defined(FSR)
	float4 resultColor = ProcessLightSources(sceneDiffuse.xyz, areaLight.color, screenWorldPosition,
		worldPosition, areaLight.position, viewStep, viewDirection);
	
	resultColor.xyz = lerp(resultColor.xyz, sceneDiffuse.xyz, sceneDiffuse.w);
	
	sceneTarget[targetCoord] = float4(resultColor.xyz, saturate(max(1.0f - resultColor.w, sceneDiffuse.w)));
#else
	float3 resultColor = ProcessLightSources(sceneDiffuse.xyz, areaLight.color, screenWorldPosition,
		worldPosition, areaLight.position, viewStep, viewDirection);
	
	resultColor = lerp(resultColor, sceneDiffuse.xyz, sceneDiffuse.w);
	
	sceneTarget[targetCoord] = float4(resultColor, sceneDiffuse.w);
	
#endif
}
