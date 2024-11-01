cbuffer MutableConstants : register(b1)
{
	float4x4 viewProj;
	
	float3 cameraPosition;
	float time;
	
	float2 mapTiling0;
	float2 mapTiling1;
	float2 mapTiling2;
	float2 mapTiling3;
	
	float zNear;
	float zFar;
	float2 padding;
	
#ifdef OUTPUT_VELOCITY
	float4x4 lastViewProjection;
#endif
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
	float4 texCoord01 : TEXCOORD1;
	float4 texCoord23 : TEXCOORD2;
	float3 worldPosition : TEXCOORD3;
#ifdef OUTPUT_VELOCITY
	float2 velocity : TEXCOORD4;
#endif
};

Output main(Input input)
{
	Output output = (Output)0;
	
	output.position = mul(viewProj, float4(input.position, 1.0f));
	output.normal = normalize(input.normal.xyz);
	output.tangent = normalize(input.tangent.xyz);
	output.binormal = cross(output.tangent, output.normal);
	output.texCoord = input.texCoord;
	output.texCoord01.xy = input.texCoord * mapTiling0;
	output.texCoord01.zw = input.texCoord * mapTiling1;
	output.texCoord23.xy = input.texCoord * mapTiling2;
	output.texCoord23.zw = input.texCoord * mapTiling3;
	output.worldPosition = input.position;
	
#ifdef OUTPUT_VELOCITY
	float4 lastNonHomogeneousPos = mul(lastViewProjection, float4(input.position, 1.0f));
	lastNonHomogeneousPos.xy /= lastNonHomogeneousPos.w;
	
	float2 nonHomogeneousPos = output.position.xy / output.position.w;
	
	output.velocity = nonHomogeneousPos - lastNonHomogeneousPos.xy;
#endif
	
	return output;
}
