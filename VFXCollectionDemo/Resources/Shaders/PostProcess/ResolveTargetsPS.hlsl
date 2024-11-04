struct Input
{
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD0;
};

struct Output
{
	float4 color : SV_TARGET0;
#if defined(FSR)
	float2 velocity : SV_TARGET1;
	float depth : SV_Depth;
#endif
};

Texture2D colorTexture : register(t0);

#if defined(FSR)
Texture2D depthVelocityTexture : register(t1);
#endif

SamplerState samplerLinear : register(s0);

Output main(Input input)
{
	Output output = (Output)0;
	
	output.color = colorTexture.SampleLevel(samplerLinear, input.texCoord, 0.0f);
	
#if defined(FSR)
	float3 depthVelocity = depthVelocityTexture.SampleLevel(samplerLinear, input.texCoord, 0.0f).xyz;
	
	output.depth = depthVelocity.x;
	output.velocity = depthVelocity.yz;
#endif
	
	return output;
}
