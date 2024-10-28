#include "Lighting/PBRLighting.hlsli"

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
	float4x4 viewProjection;
	
	float3 cameraPosition;
	float time;
	
	float2 atlasElementSize;
	float2 perlinNoiseTiling;
	
	float3 windDirection;
	float windStrength;
	
	float zNear;
	float zFar;
	float2 padding;
};

struct Input
{
	float4 position : SV_Position;
	float3 normal : NORMAL;
	float3 binormal : BINORMAL;
	float3 tangent : TANGENT;
	float2 texCoord : TEXCOORD0;
	float3 worldPosition : TEXCOORD1;
};

struct Output
{
	float4 color : SV_Target0;
};

Texture2D albedoRoughness : register(t2);
Texture2D normalAlpha : register(t3);

#ifdef AREA_LIGHT
TextureCube shadowMap : register(t4);
#else
Texture2D shadowMap : register(t4);
#endif

#if (PARTICLE_LIGHT_SOURCE_NUMBER > 0)
StructuredBuffer<PointLight> particleLightBuffer : register(t5);
#endif

SamplerState samplerLinear : register(s0);
SamplerComparisonState shadowSampler : register(s1);

[earlydepthstencil]
Output main(Input input)
{
	Output output = (Output)0;
	
	float4 texData = albedoRoughness.SampleBias(samplerLinear, input.texCoord, -1.0f);
	float3 albedo = texData.xyz;
	float roughness = texData.w * texData.w;
	
	texData = normalAlpha.SampleBias(samplerLinear, input.texCoord, -1.0f);
	float3 normal = texData.xyz * 2.0f - 1.0f.xxx;
	
	normal = BumpMapping(normal, normalize(input.normal), normalize(input.binormal), normalize(input.tangent));
	float alpha = texData.w;
	
	if (alpha < 0.5f)
		discard;
	
	Surface surface;
	surface.position = input.worldPosition;
	surface.normal = normal;
	
	Material material;
	material.albedo = albedo;
	material.f0 = albedo;
	material.f90 = 0.05f.xxx;
	material.metalness = 0.0f;
	material.roughness = roughness;
	
	float3 view = normalize(input.worldPosition - cameraPosition);
	
	float3 lightSum = 0.0f.xxx;
	
	float3 light = 0.0f.xxx;
	
#ifdef AREA_LIGHT
	ShadowCubeData shadowData = (ShadowCubeData)0;
	shadowData.zNear = zNear;
	shadowData.zFar = zFar;
	shadowData.shadowMap = shadowMap;
	shadowData.shadowSampler = shadowSampler;
	
	CalculateAreaLight(shadowData, areaLight, surface, material, view, light);
	lightSum += light;
	
	CalculateAmbientLight(ambientLight, surface, material, view, light);
	lightSum += light;
#else
	CalculateDirectionalLight(directionalLight, surface, material, view, light);
	lightSum += light;
#endif
	
#if (PARTICLE_LIGHT_SOURCE_NUMBER > 0)
	[unroll]
	for (uint particleIndex = 0u; particleIndex < PARTICLE_LIGHT_SOURCE_NUMBER; particleIndex++)
	{
		CalculatePointLight(particleLightBuffer[particleIndex], surface, material, view, light);
		lightSum += light;
	}
#endif
	
	output.color = float4(lightSum, 0.0f);
	
	return output;
}
