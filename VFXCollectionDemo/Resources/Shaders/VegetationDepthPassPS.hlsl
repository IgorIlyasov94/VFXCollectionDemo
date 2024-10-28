struct Input
{
	float4 position : SV_Position;
#ifdef DEPTH_PREPASS
	float3 normal : NORMAL;
	float3 binormal : BINORMAL;
	float3 tangent : TANGENT;
#endif
	float2 texCoord : TEXCOORD0;
#ifdef DEPTH_PREPASS
	float3 worldPosition : TEXCOORD1;
#endif
};

struct Output
{
	float depth : SV_Depth;
};

Texture2D normalAlpha : register(t2);

SamplerState samplerLinear : register(s0);

void main(Input input)
{
	float alpha = normalAlpha.SampleBias(samplerLinear, input.texCoord, -1.0f).w;
	
	if (alpha < 0.5f)
		discard;
}
