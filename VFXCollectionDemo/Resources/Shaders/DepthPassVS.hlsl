cbuffer RootConstants : register(b0)
{
	float4x4 world;
	uint lightMatrixStartIndex;
};

cbuffer LightConstants : register(b1)
{
	float4x4 lightViewProjection[LIGHT_MATRICES_NUMBER];
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
	float4 position : SV_Position;
};

Output main(Input input)
{
	Output output = (Output)0;
	
	float4 worldPosition = mul(world, float4(input.position, 1.0f));
	output.position = mul(lightViewProjection[lightMatrixStartIndex], worldPosition);
	
	return output;
}
