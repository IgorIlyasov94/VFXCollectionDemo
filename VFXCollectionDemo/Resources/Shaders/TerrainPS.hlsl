#include "Lighting/PBRLighting.hlsli"

cbuffer LightConstantBuffer : register(b0)
{
	uint32_t lightSourcesNumber;
	float3 padding;
	LightElement lights[MAX_LIGHT_SOURCE_NUMBER];
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
};

struct Output
{
	float4 color : SV_TARGET0;
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

SamplerState samplerLinear : register(s0);

[earlydepthstencil]
Output main(Input input)
{
	Output output = (Output)0;
	
	float4 blendFactor = blendMap.Sample(samplerLinear, input.texCoord);
	blendFactor = blendFactor + float4(0.001f, 0.0f, 0.0f, 0.0f);
	blendFactor /= blendFactor.x + blendFactor.y + blendFactor.z + blendFactor.w;
	
	float4 texData = albedoRoughness0.Sample(samplerLinear, input.texCoord01.xy) * blendFactor.x;
	texData += albedoRoughness1.Sample(samplerLinear, input.texCoord01.zw) * blendFactor.y;
	texData += albedoRoughness2.Sample(samplerLinear, input.texCoord23.xy) * blendFactor.z;
	texData += albedoRoughness3.Sample(samplerLinear, input.texCoord23.zw) * blendFactor.w;
	
	float3 albedo = texData.xyz;
	float roughness = texData.w * texData.w;
	
	texData = normalMetalness0.Sample(samplerLinear, input.texCoord01.xy) * blendFactor.x;
	texData += normalMetalness1.Sample(samplerLinear, input.texCoord01.zw) * blendFactor.y;
	texData += normalMetalness2.Sample(samplerLinear, input.texCoord23.xy) * blendFactor.z;
	texData += normalMetalness3.Sample(samplerLinear, input.texCoord23.zw) * blendFactor.w;
	
	float3 normal = texData.xyz * 2.0f - 1.0f.xxx;
	normal = BumpMapping(normal, input.normal, input.binormal, input.tangent);
	float metalness = texData.w;
	
	Surface surface;
	surface.position = input.worldPosition;
	surface.normal = normal;
	
	Material material;
	material.albedo = albedo;
	material.f0 = float3(0.0609f, 0.0617f, 0.0634f);
	material.f90 = 1.00f.xxx;
	material.metalness = metalness;
	material.roughness = roughness;
	
	float3 view = normalize(input.worldPosition - cameraPosition);
	
	float3 lightSum = 0.0f.xxx;
	
	for (uint lightIndex = 0; lightIndex < lightSourcesNumber; lightIndex++)
	{
		float3 light;
		CalculateLighting(lights[lightIndex], surface, material, view, light);
		lightSum += light;
	}
	
	output.color = float4(lightSum, 1.0f);
	
	return output;
}
