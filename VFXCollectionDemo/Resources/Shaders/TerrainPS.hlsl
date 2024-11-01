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
	
	float2 mapTiling0;
	float2 mapTiling1;
	float2 mapTiling2;
	float2 mapTiling3;
	
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
	float4 texCoord01 : TEXCOORD1;
	float4 texCoord23 : TEXCOORD2;
	float3 worldPosition : TEXCOORD3;
#ifdef OUTPUT_VELOCITY
	float2 velocity : TEXCOORD4;
#endif
};

struct Output
{
	float4 color : SV_TARGET0;
#ifdef OUTPUT_VELOCITY
	float2 velocity : SV_TARGET1;
#endif
};

Texture2D albedoRoughness0 : register(t0);
Texture2D albedoRoughness1 : register(t1);
Texture2D albedoRoughness2 : register(t2);
Texture2D albedoRoughness3 : register(t3);
Texture2D normalMetalness0 : register(t4);
Texture2D normalMetalness1 : register(t5);
Texture2D normalMetalness2 : register(t6);
Texture2D normalMetalness3 : register(t7);
Texture2D blendMap : register(t8);

#ifdef AREA_LIGHT
TextureCube shadowMap : register(t9);
#else
Texture2D shadowMap : register(t9);
#endif

#if (PARTICLE_LIGHT_SOURCE_NUMBER > 0)
StructuredBuffer<PointLight> particleLightBuffer : register(t10);
#endif

SamplerState samplerAnisotropic : register(s0);
SamplerComparisonState shadowSampler : register(s1);

[earlydepthstencil]
Output main(Input input)
{
	Output output = (Output)0;
	
	float4 blendFactor = blendMap.Sample(samplerAnisotropic, input.texCoord);
	blendFactor = blendFactor;
	blendFactor /= max(blendFactor.x + blendFactor.y + blendFactor.z + blendFactor.w, 0.001f);
	
	float4 texData = albedoRoughness0.Sample(samplerAnisotropic, input.texCoord01.xy) * blendFactor.x;
	texData += albedoRoughness1.Sample(samplerAnisotropic, input.texCoord01.zw) * blendFactor.y;
	texData += albedoRoughness2.Sample(samplerAnisotropic, input.texCoord23.xy) * blendFactor.z;
	texData += albedoRoughness3.Sample(samplerAnisotropic, input.texCoord23.zw) * blendFactor.w;
	
	float3 albedo = texData.xyz;
	float roughness = texData.w * texData.w;
	
	texData = normalMetalness0.Sample(samplerAnisotropic, input.texCoord01.xy) * blendFactor.x;
	texData += normalMetalness1.Sample(samplerAnisotropic, input.texCoord01.zw) * blendFactor.y;
	texData += normalMetalness2.Sample(samplerAnisotropic, input.texCoord23.xy) * blendFactor.z;
	texData += normalMetalness3.Sample(samplerAnisotropic, input.texCoord23.zw) * blendFactor.w;
	
	float3 normal = texData.xyz * 2.0f - 1.0f.xxx;
	normal = BumpMapping(normal, input.normal, input.binormal, input.tangent);
	float metalness = texData.w;
	
	Surface surface;
	surface.position = input.worldPosition;
	surface.normal = normal;
	
	Material material;
	material.albedo = albedo;
	material.f0 = float3(0.0609f, 0.0617f, 0.0634f);
	material.f90 = 0.01f.xxx;
	material.metalness = metalness;
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
	
#ifdef OUTPUT_VELOCITY
	output.velocity = input.velocity;
#endif
	
	return output;
}
