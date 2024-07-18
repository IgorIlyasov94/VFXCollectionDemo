cbuffer MutableConstants : register(b0)
{
	float4x4 invView;
	float4x4 viewProjection;
	float3 worldPosition;
	float time;
	float2 tiling0;
	float2 tiling1;
	float2 scrollSpeed0;
	float2 scrollSpeed1;
	float colorIntensity;
	float alphaSharpness;
	float distortionStrength;
	float padding;
};

struct Input
{
	float4 position : SV_Position;
	float4 color : COLOR0;
	float2 texCoord : TEXCOORD0;
	float4 texCoordScroll : TEXCOORD1;
};

struct Output
{
	float4 color : SV_TARGET0;
};

Texture2D perlinNoise : register(t0);
Texture2D haloSpectrum : register(t1);

SamplerState samplerLinear : register(s0);

Output main(Input input)
{
	Output output = (Output)0;
	
	float2 distortion = perlinNoise.Sample(samplerLinear, input.texCoordScroll.xy).xy * 2.0f - 1.0f.xx;
	distortion *= distortionStrength;
	
	float4 noise = perlinNoise.Sample(samplerLinear, input.texCoordScroll.zw);
	
	float4 spectrum = haloSpectrum.Sample(samplerLinear, input.texCoord + distortion);
	
	float fading = saturate((input.color.r * input.color.g * input.color.w) * 4.0f);
	float alpha = pow(fading * spectrum.w * (noise.x * 0.5f + 0.5f), alphaSharpness);
	
	output.color = float4(spectrum.xyz * colorIntensity, saturate(alpha));
	
	return output;
}
