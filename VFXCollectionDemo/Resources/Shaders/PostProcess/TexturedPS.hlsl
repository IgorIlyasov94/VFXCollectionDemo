struct Input
{
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD0;
};

struct Output
{
	float4 color : SV_TARGET0;
};

Texture2D colotTexture : register(t0);
SamplerState samplerLinear : register(s0);

Output main(Input input)
{
	Output output = (Output)0;
	
	float3 color = colotTexture.SampleLevel(samplerLinear, input.texCoord, 0.0f).xyz;
	
	output.color = float4(color, 1.0f);
	
	return output;
}
