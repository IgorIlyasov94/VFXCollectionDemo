cbuffer MutableConstants : register(b0)
{
	float4x4 invView;
	float4x4 viewProjection;
	float4 color0;
	float4 color1;
	float3 worldPosition;
	float time;
	float2 tiling0;
	float2 tiling1;
	float2 scrollSpeed0;
	float2 scrollSpeed1;
	float colorIntensity;
	float alphaSharpness;
	float distortionStrength;
	float padding;
};

struct Input
{
	float3 position : POSITION;
	float4 color : COLOR0;
	float2 texCoord : TEXCOORD0;
};

struct Output
{
	float4 position : SV_Position;
	float4 color : COLOR0;
	float4 texCoord : TEXCOORD0;
};

Output main(Input input)
{
	Output output = (Output)0;
	
	float4 worldPos = float4(mul((float3x3)invView, input.position), 1.0f);
	worldPos.xyz += worldPosition;
	
	output.position = mul(viewProjection, worldPos);
	output.color = input.color;
	
	float2 offset0 = frac(time * scrollSpeed0);
	float2 offset1 = frac(time * scrollSpeed1);
	
	output.texCoord.xy = input.texCoord * tiling0;
	output.texCoord.zw = input.texCoord * tiling1;
	output.texCoord.xy += offset0;
	output.texCoord.zw += offset1;
	
	return output;
}
