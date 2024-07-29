cbuffer RootConstants : register(b0)
{
	float4x4 world;
	uint lightMatrixStartIndex;
};

struct Input
{
	float3 position : POSITION;
	half4 normal : NORMAL;
	half4 tangent : TANGENT;
	half2 texCoord : TEXCOORD0;
};

struct Output
{
	float4 position : POSITION;
};

Output main(Input input)
{
	Output output = (Output)0;
	output.position = mul(world, float4(input.position, 1.0f));
	
	return output;
}
