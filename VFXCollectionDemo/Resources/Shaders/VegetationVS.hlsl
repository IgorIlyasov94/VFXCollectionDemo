struct Vegetation
{
	float4x4 world;
	float2 atlasElementOffset;
	float2 padding;
};

cbuffer MutableConstants : register(b0)
{
	float4x4 viewProjection;
	
	float3 cameraDirection;
	float time;
	
	float2 atlasElementSize;
	float2 padding;
};

struct Input
{
	float3 position : POSITION;
	float2 texCoord : TEXCOORD0;
	uint instanceId : SV_InstanceID;
	bool isFrontFace : SV_IsFrontFace;
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

StructuredBuffer<Vegetation> vegetationBuffer : register(t0);
Texture2D perlinNoise : register(t1);

SamplerState samplerLinear : register(s0);

Output main(Input input)
{
	Output output = (Output)0;
	
	Vegetation vegetation = vegetationBuffer[input.instanceId];
	
	float4 worldPosition = float4(mul(vegetation.world, float3(input.position, 1.0f)), 1.0f);
	output.position = mul(viewProjection, worldPosition);
	
	float sideCoeff = input.isFrontFace ? -1.0f : 1.0f;
	
	output.normal = normalize(mul((float3x3)world, float3(0.0f, sideCoeff, 0.0f)));
	output.tangent = normalize(mul((float3x3)world, float3(sideCoeff, 0.0f, 0.0f)));
	
	output.binormal = cross(output.tangent, output.normal);
	output.texCoord = input.texCoord * atlasElementSize + vegetation.atlasElementOffset;
	output.worldPosition = worldPosition.xyz;
	
	return output;
}
