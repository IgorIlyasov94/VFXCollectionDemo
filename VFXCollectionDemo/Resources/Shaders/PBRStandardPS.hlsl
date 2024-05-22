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

[earlydepthstencil]
Output main(Input input)
{
	Output output = (Output)0;
	
	output.color = float4(input.texCoord, 0.0f, 1.0f);
	
	return output;
}
