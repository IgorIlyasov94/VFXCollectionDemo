cbuffer MutableConstants : register(b0)
{
	float4x4 worldViewProj;
};

struct Input
{
	float4 position : SV_Position;
	float3 normal : NORMAL;
	float2 texCoord : TEXCOORD0;
};

struct Output
{
	float4 color : SV_TARGET0;
};

Texture2D albedoRoughness : register(t0);
Texture2D normalMetalness : register(t1);

SamplerState samplerLinear : register(s0);

[earlydepthstencil]
Output main(Input input)
{
	Output output = (Output)0;
	
	float4 texData = albedoRoughness.Sample(samplerLinear, input.texCoord);
	float3 albedo = texData.xyz;
	float roughness = texData.w * texData.w;
	
	texData = normalMetalness.Sample(samplerLinear, input.texCoord);
	float3 normal = texData.xyz * 2.0f - 1.0f.xxx;
	float metalness = texData.w;
	
	output.color = float4(albedo, 1.0f);
	
	return output;
}
