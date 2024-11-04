static const float3 STRENGTH_COEFF = float3(1.0f, 0.6f, 0.2f);

cbuffer MutableConstants : register(b0)
{
	float4x4 invView;
	float4x4 viewProjection;
	float3 color0;
	float colorIntensity;
	float3 color1;
	float distortionStrength;
	float3 worldPosition;
	float time;
	float2 tiling0;
	float2 tiling1;
	float2 scrollSpeed0;
	float2 scrollSpeed1;
};

struct Input
{
	float4 position : SV_Position;
	float4 color : COLOR0;
	float4 texCoord : TEXCOORD0;
	float3 worldPos : TEXCOORD2;
};

struct Output
{
	float4 color : SV_TARGET0;
};

Texture2D spectrumMap : register(t0);
Texture3D volumeNoise : register(t1);
Texture2D perlinNoise : register(t2);

SamplerState samplerLinear : register(s0);

Output main(Input input)
{
	Output output = (Output)0;
	
	float3 volumeCoord = all(input.worldPos) ? normalize(input.worldPos) : 0.0f.xxx;
	float dist = length(input.worldPos) * 0.5f;
	
	float noise = volumeNoise.Sample(samplerLinear, volumeCoord).x;
	
	float alpha = saturate(1.0f - dist * noise * input.color.r * 10.0f) * pow(input.color.r, 4.0f);
	
	float2 distortion = perlinNoise.Sample(samplerLinear, input.texCoord.xy).xy * 2.0f - 1.0f.xx;
	distortion *= distortionStrength;
	
	float3 spectrum = spectrumMap.Sample(samplerLinear, input.texCoord.wz + distortion).xyz;
	
	dist = pow(dist, 0.075f);
	float3 color = lerp(color0, spectrum * color1, saturate(dist));
	
	output.color = float4(color * colorIntensity, saturate(alpha));
	
	return output;
}
