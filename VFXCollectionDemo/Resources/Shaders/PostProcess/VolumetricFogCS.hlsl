#include "../Lighting/PBRLighting.hlsli"

static const uint NUM_THREADS = 64;
static const uint NUM_VIEW_STEPS = 8;
static const uint NUM_LIGHT_STEPS = 1;

static const float MAX_DISTANCE = 10.0f;
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
	
	float3 cameraPosition;
	float distanceFalloffExponent;
	
	float distanceFalloffStart;
	float distanceFalloffLength;
	float fogTiling;
	float fogOffset;
};

struct Input
{
	uint3 dispatchThreadId : SV_DispatchThreadID;
	uint3 groupId : SV_GroupID;
	uint groupIndex : SV_GroupIndex;
};

Texture3D<float4> fogMap : register(t0);
Texture2D<float> sceneDepth : register(t1);

#if (PARTICLE_LIGHT_SOURCE_NUMBER > 0)
StructuredBuffer<PointLight> particleLightBuffer : register(t2);
#endif

RWStructuredBuffer<float4> sceneBuffer : register(u0);

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
	return saturate(pow((sceneDistance - distanceFalloffStart) / distanceFalloffLength, distanceFalloffExponent));
}

float3 ProcessLightSource(float3 sceneDiffuse, float3 lightColor, float3 samplePoint,
	float3 lightPosition, float3 viewStep, float3 viewDir, float viewStepLength)
{
	float3 resultColor = sceneDiffuse;
	float3 vWayPoint = samplePoint;
	
	float3 offset = float3(frac(fogOffset), 0.0f, 0.0f);
	
	[unroll]
	for (uint vStepIndex = 0u; vStepIndex < NUM_VIEW_STEPS; vStepIndex++)
	{
		vWayPoint += viewStep;
		
		float3 fogMapDistortion = fogMap.SampleLevel(samplerLinear, vWayPoint * 0.03f, 0.0f).xyz * 2.0f - 1.0f;
		fogMapDistortion *= 0.1f;
		float fogAlpha = fogMap.SampleLevel(samplerLinear, vWayPoint * fogTiling + offset + fogMapDistortion, 0.0f).w;
		float3 absorptionCoeff = lerp(AIR_WET_ABSORPTION_COEFF, WATER_ABSORPTION_COEFF, fogAlpha) * 200.0f;
		resultColor = BouguerLambertBeerLaw(resultColor, viewStepLength, absorptionCoeff);
		
		float3 lightDir = vWayPoint - lightPosition;
		float3 lightStep = lightDir / (NUM_LIGHT_STEPS + 1);
		float lightStepLength = length(lightStep);
		lightDir = normalize(lightDir + 0.0000001f.xxx);
		
		float3 lWayPoint = samplePoint;
		
		float3 scatteredLight = lightColor;
		
		[unroll]
		for (uint lStepIndex = 0u; lStepIndex < NUM_LIGHT_STEPS; lStepIndex++)
		{
			lWayPoint -= lightStep;
			
			fogMapDistortion = fogMap.SampleLevel(samplerLinear, lWayPoint * 0.03f, 0.0f).xyz * 2.0f - 1.0f;
			fogMapDistortion *= 0.1f;
			fogAlpha = fogMap.SampleLevel(samplerLinear, lWayPoint * fogTiling + offset + fogMapDistortion, 0.0f).w;
			absorptionCoeff = lerp(AIR_WET_ABSORPTION_COEFF, WATER_ABSORPTION_COEFF, fogAlpha) * 200.0f;
			scatteredLight = BouguerLambertBeerLaw(scatteredLight, lightStepLength, absorptionCoeff);
		}
		
		float cosAngle = dot(viewDir, -lightDir);
		float phaseValue = DoubleHenyeyGreensteinPhase(cosAngle);
		
		resultColor = scatteredLight * saturate(phaseValue);
	}
	
	return resultColor / (NUM_LIGHT_STEPS * NUM_VIEW_STEPS);
}

[numthreads(NUM_THREADS, 1, 1)]
void main(Input input)
{
	uint bufferIndex = input.dispatchThreadId.x;
	
	if (bufferIndex >= area)
		return;
	
	float2 texCoord;
	texCoord.x = (bufferIndex % widthU) / width;
	texCoord.y = (bufferIndex / widthU) / height;
	
	float depth = sceneDepth.SampleLevel(samplerLinear, texCoord, 0.0f).x;
	float3 worldPosition = ReconstructWorldPosition(texCoord, depth, invViewProjection);
	float3 screenWorldPosition = ReconstructWorldPosition(texCoord, 0.0f, invViewProjection);
	
	//float worldPositionLength = length(worldPosition);
	//worldPosition = worldPositionLength > MAX_DISTANCE ? worldPosition * (MAX_DISTANCE / worldPositionLength) : worldPosition;
	
	float3 viewStep = worldPosition - screenWorldPosition;//cameraPosition;
	float sceneDistance = length(viewStep);
	float3 viewDir = viewStep / sceneDistance;
	//viewStep /= NUM_VIEW_STEPS + 1;
	viewStep = viewDir * 1.5f;
	float viewStepLength = length(viewStep);
	
	float falloff = CalculateVisibilityFalloff(sceneDistance);
	
	float3 sceneDiffuse = sceneBuffer[bufferIndex].xyz;
	
	float3 resultColor = ProcessLightSource(sceneDiffuse, areaLight.color, screenWorldPosition, areaLight.position, viewStep, viewDir, viewStepLength);
	resultColor = lerp(sceneDiffuse, resultColor, falloff);
	
	sceneBuffer[bufferIndex] = float4(resultColor, 1.0f);
}
