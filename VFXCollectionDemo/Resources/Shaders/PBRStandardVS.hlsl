cbuffer MutableConstants : register(b0)
{
	float4x4 worldViewProj;
};

struct Input
{
	float3 position : POSITION;
	half4 normal : NORMAL;
	half2 texCoord : TEXCOORD0;
	uint vertexId : SV_VertexID;
};

struct Output
{
	float4 position : SV_Position;
	float3 normal : NORMAL;
	float2 texCoord : TEXCOORD0;
};

Output main(Input input)
{
	Output output = (Output)0;
	
	output.position = mul(worldViewProj, float4(input.position, 1.0f));
	output.normal = input.normal.xyz;
	output.texCoord = input.texCoord;
	
	return output;
}
