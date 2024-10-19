cbuffer MutableConstants : register(b0)
{
	float4x4 invView;
	float4x4 viewProjection;
	
	float4 color0;
	float4 color1;
	
	float3 worldPosition;
	float colorIntensity;
	
	float2 minSize;
	float2 maxSize;
	
	float cosTime;
	float3 padding;
};

struct Input
{
	float3 position : POSITION;
	float2 texCoord : TEXCOORD0;
};

struct Output
{
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD0;
};

Output main(Input input)
{
	Output output = (Output)0;
	
	float3 localPos = input.position;
	localPos.xy *= lerp(minSize, maxSize, cosTime);
	
	float4 worldPos = float4(mul((float3x3)invView, localPos), 1.0f);
	worldPos.xyz += worldPosition;
	
	output.position = mul(viewProjection, worldPos);
	output.texCoord = input.texCoord;
	return output;
}
