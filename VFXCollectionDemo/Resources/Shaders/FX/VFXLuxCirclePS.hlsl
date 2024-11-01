static const float3 STRENGTH_COEFF = float3(1.0f, 0.6f, 0.2f);

cbuffer MutableConstants : register(b0)
{
	float4x4 invView;
	float4x4 viewProjection;
	float4 color0;
	float4 color1;
	float3 worldPosition;
	float time;
	float2 tiling0;
	float2 tiling1;
	float2 scrollSpeed0;
	float2 scrollSpeed1;
	float colorIntensity;
	float alphaSharpness;
	float spectralTransitionSharpness;
	float distortionStrength;
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
	
	float2 distortion = perlinNoise.Sample(samplerLinear, input.texCoord.xy).xy * 2.0f - 1.0f.xx;
	distortion *= distortionStrength;
	
	float noiseR = perlinNoise.Sample(samplerLinear, input.texCoord.zw + distortion * STRENGTH_COEFF.x).x;
	float noiseG = perlinNoise.Sample(samplerLinear, input.texCoord.zw + distortion * STRENGTH_COEFF.y).x;
	float noiseB = perlinNoise.Sample(samplerLinear, input.texCoord.zw + distortion * STRENGTH_COEFF.z).x;
	
	float fading = input.color.r * input.color.g * input.color.w * 3.0f;
	
	float alpha =  pow(fading * max(noiseR, max(noiseG, noiseB)), alphaSharpness);
	
	float3 color = lerp(color0.xyz, color1.xyz, saturate(fading));
	color = lerp(float3(noiseR, noiseG, noiseB), color, pow(1.0f - input.color.g, spectralTransitionSharpness));
	
	output.color = float4(color * colorIntensity, saturate(alpha));
	
	return output;
}
