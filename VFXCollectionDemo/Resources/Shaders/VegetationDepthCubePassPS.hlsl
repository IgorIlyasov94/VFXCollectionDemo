struct Input
{
	float4 position : SV_Position;
	uint targetIndex : SV_RenderTargetArrayIndex;
	float2 texCoord : TEXCOORD0;
};

Texture2D normalAlpha : register(t2);

SamplerState samplerLinear : register(s0);

void main(Input input)
{
	float alpha = normalAlpha.SampleBias(samplerLinear, input.texCoord, -1.0f).w;
	
	if (alpha < 0.5f)
		discard;
}
