cbuffer RootConstants : register(b0)
{
#ifdef DEPTH_PREPASS
	float4x4 worldViewProjection;
#else
	float4x4 world;
	uint lightMatrixStartIndex;
#endif
};

#ifndef DEPTH_PREPASS
cbuffer LightConstants : register(b1)
{
	float4x4 lightViewProjection[LIGHT_MATRICES_NUMBER];
};
#endif

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
	
#ifdef DEPTH_PREPASS
	output.position = mul(worldViewProjection, float4(input.position, 1.0f));
#else
	float4 worldPosition = mul(world, float4(input.position, 1.0f));
	output.position = mul(lightViewProjection[lightMatrixStartIndex], worldPosition);
#endif
	
	return output;
}
