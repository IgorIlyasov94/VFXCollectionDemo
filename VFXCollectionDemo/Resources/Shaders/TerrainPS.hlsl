#include "PBRLighting.hlsli"

cbuffer MutableConstants : register(b0)
{
	float4x4 viewProj;
	
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
	
	PointLight pointLight;
	pointLight.position = float3(sin(time * 3.0f) * 0.3f, 0.0f, 3.0f);
	pointLight.color = lerp(float3(4.2f, 3.9f, 2.774f), float3(4.1f, 3.9f, 2.8f), 0.0f) * 1.0f;
	pointLight.color *= saturate(cos(time * 3.0f) * 0.2f + 0.8f);
	
	LightingDesc lightingDesc;
	lightingDesc.surface = surface;
	lightingDesc.pointLight = pointLight;
	lightingDesc.material = material;
	
	float3 light;
	CalculateLighting(lightingDesc, view, light);
	
	lightSum += max(light, 0.0f.xxx);
	
	float3 ambient = lerp(float3(0.4f, 0.35f, 0.1f), float3(0.5f, 0.6f, 0.65f), saturate(normal.z * 0.5f + 0.5f));
	
	output.color = float4(lightSum + albedo * ambient * 0.01f, 1.0f);
	
	return output;
}
