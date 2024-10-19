#include "../Lighting/PBRLighting.hlsli"

static const uint NUM_THREADS = 64;
static const uint NUM_VIEW_STEPS = 4;
static const uint NUM_LIGHT_STEPS = 2;

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
	float padding;
};

struct Input
{
	uint3 dispatchThreadId : SV_DispatchThreadID;
	uint3 groupId : SV_GroupID;
	uint groupIndex : SV_GroupIndex;
};

Texture3D<float> fogMap : register(t0);
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
	
	[unroll]
	for (uint vStepIndex = 0u; vStepIndex < NUM_VIEW_STEPS; vStepIndex++)
	{
		vWayPoint -= viewStep;
		
		float fogAlpha = fogMap.SampleLevel(samplerLinear, vWayPoint * fogTiling, 0.0f).x;
		float3 absorptionCoeff = lerp(AIR_WET_ABSORPTION_COEFF, WATER_ABSORPTION_COEFF, fogAlpha);
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
			
			fogAlpha = fogMap.SampleLevel(samplerLinear, lWayPoint * fogTiling, 0.0f).x;
			absorptionCoeff = lerp(AIR_WET_ABSORPTION_COEFF, WATER_ABSORPTION_COEFF, fogAlpha);
			scatteredLight = BouguerLambertBeerLaw(scatteredLight, lightStepLength, absorptionCoeff);
		}
		
		float cosAngle = dot(viewDir, -lightDir);
		float phaseValue = DoubleHenyeyGreensteinPhase(cosAngle);
		
		resultColor += scatteredLight * saturate(phaseValue);
	}
	
	return resultColor;
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
	
	float3 viewStep = worldPosition - cameraPosition;
	float sceneDistance = length(viewStep);
	float3 viewDir = viewStep / sceneDistance;
	viewStep /= NUM_VIEW_STEPS + 1;
	float viewStepLength = length(viewStep);
	
	float falloff = CalculateVisibilityFalloff(sceneDistance);
	
	float3 sceneDiffuse = sceneBuffer[bufferIndex].xyz;
	
	float3 resultColor = ProcessLightSource(sceneDiffuse, areaLight.color, worldPosition, areaLight.position, viewStep, viewDir, viewStepLength);
	
	sceneBuffer[bufferIndex] = float4(resultColor, 1.0f);
}
