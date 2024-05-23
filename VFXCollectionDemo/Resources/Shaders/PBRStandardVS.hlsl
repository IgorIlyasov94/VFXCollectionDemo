cbuffer MutableConstants : register(b0)
{
	float4x4 world;
	float4x4 viewProj;
	float4 cameraDirection;
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
	float3 normal : NORMAL;
	float3 binormal : BINORMAL;
	float3 tangent : TANGENT;
	float2 texCoord : TEXCOORD0;
	float3 worldPosition : TEXCOORD1;
};

Output main(Input input)
{
	Output output = (Output)0;
	
	float4 worldPosition = mul(world, float4(input.position, 1.0f));
	
	output.position = mul(viewProj, worldPosition);
	output.normal = normalize(mul((float3x3)world, input.normal.xyz));
	output.tangent = normalize(mul((float3x3)world, input.tangent.xyz));
	output.binormal = cross(output.tangent, output.normal);
	output.texCoord = input.texCoord;
	output.worldPosition = worldPosition.xyz;
	
	return output;
}
