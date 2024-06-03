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
	float3 position : POSITION;
	float4 color : COLOR0;
	float2 texCoord : TEXCOORD0;
};

struct Output
{
	float4 position : SV_Position;
	float4 color : COLOR0;
	float4 texCoord : TEXCOORD0;
};

Texture2D perlinNoise : register(t0);

SamplerState samplerLinear : register(s0);

Output main(Input input)
{
	Output output = (Output)0;
	
	float2 offset0 = frac(time * scrollSpeed0);
	float2 offset1 = frac(time * scrollSpeed1);
	
	output.texCoord.xy = input.texCoord * tiling0 + offset0;
	output.texCoord.zw = input.texCoord * tiling1 + offset1;
	
	float4 localScroll = input.texCoord.yyyy;
	localScroll.xy = localScroll.xy * tiling0 + offset0;
	localScroll.zw = localScroll.zw * tiling1 + offset1;
	
	float4 noise;
	noise.x = perlinNoise.SampleLevel(samplerLinear, localScroll.xy, 1.0f).x * 2.0f - 1.0f;
	noise.y = perlinNoise.SampleLevel(samplerLinear, localScroll.yz, 2.0f).y * 2.0f - 1.0f;
	noise.z = perlinNoise.SampleLevel(samplerLinear, localScroll.zw, 3.0f).z * 2.0f - 1.0f;
	noise.w = perlinNoise.SampleLevel(samplerLinear, localScroll.wx, 4.0f).w * 2.0f - 1.0f;
	
	float4 localPosition = float4(input.position, 1.0f);
	localPosition.x += dot(noise, displacementStrength) * input.color.y;
	
	float4 worldPosition = mul(world, localPosition);
	
	output.position = mul(viewProjection, worldPosition);
	output.color = input.color;
	
	return output;
}
