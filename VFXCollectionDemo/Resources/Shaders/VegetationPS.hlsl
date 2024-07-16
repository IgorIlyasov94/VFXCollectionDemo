#include "PBRLighting.hlsli"

cbuffer MutableConstants : register(b0)
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
	
	normal = BumpMapping(normal, input.normal, input.binormal, input.tangent);
	float alpha = texData.w;
	
	if (alpha < 0.5f)
		discard;
	
	Surface surface;
	surface.position = input.worldPosition;
	surface.normal = normal;
	
	Material material;
	material.albedo = albedo;
	material.f0 = float3(0.175f, 0.4f, 0.075f);
	material.f90 = 1.00f.xxx;
	material.metalness = 0.0f;
	material.roughness = roughness;
	
	float3 view = normalize(input.worldPosition - cameraPosition);
	
	float3 lightSum = 0.0f.xxx;
	
	PointLight pointLight;
	pointLight.position = float3(sin(time * 3.0f) * 0.3f, 0.0f, 5.0f);
	pointLight.color = lerp(float3(4.2f, 3.9f, 2.774f), float3(4.1f, 3.9f, 2.8f), 0.0f) * 2.0f;
	pointLight.color *= saturate(cos(time * 3.0f) * 0.2f + 0.8f);
	
	LightingDesc lightingDesc;
	lightingDesc.surface = surface;
	lightingDesc.pointLight = pointLight;
	lightingDesc.material = material;
	
	float3 light;
	CalculateLighting(lightingDesc, view, light);
	
	lightSum += light;
	
	float3 ambient = lerp(float3(0.4f, 0.35f, 0.1f), float3(0.5f, 0.6f, 0.65f), saturate(normal.z * 0.5f + 0.5f));
	
	output.color = float4(lightSum + albedo * ambient * 0.01f, 1.0f);
	
	return output;
}
