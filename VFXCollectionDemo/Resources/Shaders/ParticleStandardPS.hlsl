cbuffer MutableConstants : register(b0)
{
	float4x4 invView;
	float4x4 viewProjection;
	
	float2 atlasElementOffset;
	float2 atlasElementSize;
	
	float colorIntensity;
	float3 padding;
};

struct Input
{
	float4 position : SV_Position;
	float4 color : COLOR0;
	float2 texCoord : TEXCOORD0;
};

struct Output
{
	float4 color : SV_TARGET0;
};

Texture2D vfxAtlas : register(t2);

SamplerState samplerLinear : register(s0);

Output main(Input input)
{
	Output output = (Output)0;
	
	float mask = vfxAtlas.Sample(samplerLinear, input.texCoord).x;
	
	float4 color = input.color;
	color.xyz *= colorIntensity;
	color.w *= mask;
	
	output.color = color;
	
	return output;
}
