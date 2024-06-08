struct Input
{
	float3 position : POSITION;
	half2 texCoord : TEXCOORD0;
};

struct Output
{
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD0;
};

Output main(Input input)
{
	Output output = (Output)0;
	
	output.position = float4(input.position, 1.0f);
	output.texCoord = input.texCoord;
	
	return output;
}
