cbuffer MutableConstants : register(b0)
{
	float4x4 invView;
	float4x4 viewProjection;
	
	float2 atlasElementOffset;
	float2 atlasElementSize;
	
	float2 noiseTiling;
	float2 noiseScrollSpeed;
	
	float time;
	float deltaTime;
	float particleNumber;
	float noiseStrength;
	
	float velocityStrength;
	float3 padding;
};

struct Input
{
	float4 position : SV_Position;
	float4 texCoord : TEXCOORD0;
	float3 velocityAlpha : TEXCOORD1;
};

struct Output
{
	float2 distortion : SV_TARGET0;
};

Texture2D vfxAtlas : register(t2);
Texture2D perlinNoise : register(t3);

SamplerState samplerLinear : register(s0);

Output main(Input input)
{
	Output output = (Output)0;
	
	float mask = vfxAtlas.Sample(samplerLinear, input.texCoord.xy).x;
	float2 distortion = perlinNoise.Sample(samplerLinear, input.texCoord.zw).xy;
	distortion = distortion * 2.0f - 1.0f.xx;
	
	float distortionCoeff = mask * input.velocityAlpha.z;
	
	output.distortion = input.velocityAlpha.xy * velocityStrength;
	output.distortion += distortion * noiseStrength;
	output.distortion *= distortionCoeff;
	
	return output;
}
