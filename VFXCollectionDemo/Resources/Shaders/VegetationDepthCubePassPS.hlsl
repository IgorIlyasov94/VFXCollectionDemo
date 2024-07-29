struct Input
{
	float4 position : SV_Position;
	uint targetIndex : SV_RenderTargetArrayIndex;
	float2 texCoord : TEXCOORD0;
	float2 depth : TEXCOORD1;
};

struct Output
{
	float depth : SV_Depth;
};

Texture2D normalAlpha : register(t2);

SamplerState samplerLinear : register(s0);

Output main(Input input)
{
	Output output = (Output)0;
	
	float alpha = normalAlpha.SampleBias(samplerLinear, input.texCoord, -1.0f).w;
	
	if (alpha < 0.5f)
		discard;
	
	output.depth = input.depth.x / input.depth.y;
	
	return output;
}
