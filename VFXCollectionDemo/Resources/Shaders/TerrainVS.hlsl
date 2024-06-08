cbuffer MutableConstants : register(b0)
{
	float4x4 viewProj;
	
	float3 cameraDirection;
	float time;
	
	float2 mapTiling0;
	float2 mapTiling1;
	float2 mapTiling2;
	float2 mapTiling3;
};

struct Input
{
	float3 position : POSITION;
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
	float2 texCoord : TEXCOORD0;
	float4 blendFactor : BLENDWEIGHT;
};

struct Output
{
	float4 position : SV_Position;
	float3 normal : NORMAL;
	float3 binormal : BINORMAL;
	float3 tangent : TANGENT;
	float4 texCoord01 : TEXCOORD0;
	float4 texCoord23 : TEXCOORD1;
	float3 worldPosition : TEXCOORD2;
	float4 blendFactor : TEXCOORD3;
};

Output main(Input input)
{
	Output output = (Output)0;
	
	output.position = mul(viewProj, float4(input.position, 1.0f));
	output.normal = input.normal.xyz;
	output.tangent = input.tangent.xyz;
	output.binormal = cross(output.tangent, output.normal);
	output.texCoord01.xy = input.texCoord * mapTiling0;
	output.texCoord01.zw = input.texCoord * mapTiling1;
	output.texCoord23.xy = input.texCoord * mapTiling2;
	output.texCoord23.zw = input.texCoord * mapTiling3;
	output.worldPosition = worldPosition.xyz;
	output.blendFactor = input.blendFactor;
	
	return output;
}
