struct Input
{
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD0;
};

struct Output
{
	float alpha : SV_TARGET0;
};

Texture2D<float4> sceneColor : register(t0);

Output main(Input input)
{
	Output output = (Output)0;
	
	output.alpha = saturate(sceneColor.Load(int3((int2)input.position.xy, 0)).w);
	
	return output;
}
