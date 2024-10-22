cbuffer MutableConstants : register(b0)
{
	float4x4 invView;
	float4x4 viewProjection;
	
	float4 color0;
	float4 color1;
	
	float3 worldPosition;
	float colorIntensity;
	
	float2 minSize;
	float2 maxSize;
	
	float cosTime;
	float3 padding;
};

struct Input
{
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD0;
};

struct Output
{
	float4 color : SV_TARGET0;
};

Texture2D colorTexture : register(t0);
SamplerState samplerLinear : register(s0);

Output main(Input input)
{
	Output output = (Output)0;
	
	float mask = colorTexture.SampleBias(samplerLinear, input.texCoord, -1.0f).x;
	
	float3 color = lerp(color0.xyz, color1.xyz, mask) * colorIntensity * mask;
	
	output.color = float4(color, mask);
	
	return output;
}
