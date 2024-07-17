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
	
	float2 atlasElementSize;
	float2 perlinNoiseTiling;
	
	float3 windDirection;
	float windStrength;
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
	float4 color : SV_TARGET0;
};

Texture2D albedoRoughness : register(t2);
Texture2D normalMetalness : register(t3);

SamplerState samplerLinear : register(s0);

Output main(Input input)
{
	Output output = (Output)0;
	
	float4 texData = albedoRoughness.SampleBias(samplerLinear, input.texCoord, -1.0f);
	float3 albedo = texData.xyz;
	float roughness = texData.w * texData.w;
	
	texData = normalMetalness.SampleBias(samplerLinear, input.texCoord, -1.0f);
	float3 normal = texData.xyz * 2.0f - 1.0f.xxx;
	
	normal = normalize(input.normal);//BumpMapping(normal, normalize(input.normal), normalize(input.binormal), normalize(input.tangent));
	float alpha = texData.w;
	
	if (alpha < 0.5f)
		discard;
	
	Surface surface;
	surface.position = input.worldPosition;
	surface.normal = normal;
	
	Material material;
	material.albedo = albedo;
	material.f0 = float3(0.175f, 0.4f, 0.075f)*2;
	material.f90 = 1.00f.xxx;
	material.metalness = 0.0f;
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
