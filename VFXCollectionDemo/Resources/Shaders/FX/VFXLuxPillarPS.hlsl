static const float3 LUMINANCE_VECTOR = float3(0.2126f, 0.7152f, 0.0722f);

cbuffer MutableConstants : register(b0)
{
	float4x4 world;
	float4x4 viewProjection;
	float2 tiling0;
	float2 tiling1;
	float2 scrollSpeed0;
	float2 scrollSpeed1;
	float4 displacementStrength;
	float4 color0;
	float4 color1;
	float alphaIntensity;
	float colorIntensity;
	float time;
	float padding;
};

struct Input
{
	float4 position : SV_Position;
	float4 color : COLOR0;
	float4 texCoord : TEXCOORD0;
};

struct Output
{
	float4 color : SV_TARGET0;
};

Texture2D perlinNoise : register(t0);

SamplerState samplerLinear : register(s0);

Output main(Input input)
{
	Output output = (Output)0;
	
	float4 noise0 = perlinNoise.Sample(samplerLinear, input.texCoord.xy);
	float4 noise1 = perlinNoise.Sample(samplerLinear, input.texCoord.zw);
	
	float alpha = input.color.w * lerp(noise0.x, noise1.y, noise0.y * noise1.z * noise1.w) / 0.5f;
	alpha = pow(saturate(alpha), alphaIntensity);
	float3 color = lerp(color0.xyz, color1.xyz, alpha);
	
	output.color = float4(color * colorIntensity, alpha);
	
	return output;
}
