#include "PBRLighting.hlsli"

cbuffer MutableConstants : register(b0)
{
	float4x4 viewProj;
	
	float3 cameraDirection;
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
	float4 texCoord01 : TEXCOORD0;
	float4 texCoord23 : TEXCOORD1;
	float3 worldPosition : TEXCOORD2;
	float4 blendFactor : TEXCOORD3;
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

SamplerState samplerLinear : register(s0);

[earlydepthstencil]
Output main(Input input)
{
	Output output = (Output)0;
	
	float4 texData = albedoRoughness0.Sample(samplerLinear, input.texCoord01.xy) * input.blendFactor.x;
	texData += albedoRoughness1.Sample(samplerLinear, input.texCoord01.zw) * input.blendFactor.y;
	texData += albedoRoughness2.Sample(samplerLinear, input.texCoord23.xy) * input.blendFactor.z;
	texData += albedoRoughness3.Sample(samplerLinear, input.texCoord23.zw) * input.blendFactor.w;
	
	float3 albedo = texData.xyz;
	float roughness = texData.w * texData.w;
	
	texData = normalMetalness0.Sample(samplerLinear, input.texCoord01.xy) * input.blendFactor.x;
	texData += normalMetalness1.Sample(samplerLinear, input.texCoord01.zw) * input.blendFactor.y;
	texData += normalMetalness2.Sample(samplerLinear, input.texCoord23.xy) * input.blendFactor.z;
	texData += normalMetalness3.Sample(samplerLinear, input.texCoord23.zw) * input.blendFactor.w;
	
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
	
	float3 lightSum = 0.0f.xxx;
	
	for (int i = 0; i < 8; i++)
	{
		PointLight pointLight;
		pointLight.position = float3(sin(time * 3.0f + i * 1.28f) * 0.3f * i, 0.0f, 0.686f * i + 0.2f);
		pointLight.color = lerp(float3(4.2f, 3.9f, 2.774f), float3(4.1f, 3.9f, 2.8f), i / 7.0f);
		pointLight.color *= saturate(cos(time * 3.0f + i * 1.28f) * 0.1f * i + 0.1f);
		
		LightingDesc lightingDesc;
		lightingDesc.surface = surface;
		lightingDesc.pointLight = pointLight;
		lightingDesc.material = material;
		
		float3 light;
		CalculateLighting(lightingDesc, cameraDirection, light);
		
		lightSum += light;
	}
	
	float3 ambient = lerp(float3(0.4f, 0.35f, 0.1f), float3(0.5f, 0.6f, 0.65f), saturate(normal.z * 0.5f + 0.5f));
	
	output.color = float4(lightSum + albedo * ambient * 0.01f, 1.0f);
	
	return output;
}
