cbuffer MutableConstants : register(b1)
{
	float4x4 viewProjection;
	
	float3 cameraPosition;
	float time;
	
	float2 atlasElementSize;
	float2 perlinNoiseTiling;
	
	float3 windDirection;
	float windStrength;
	
	float zNear;
	float zFar;
	float lastTime;
	float mipBias;
	
	float3 lastWindDirection;
	float lastWindStrength;
	
	float4x4 lastViewProjection;
};

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
#ifdef FSR
	float alpha = normalAlpha.SampleBias(samplerLinear, input.texCoord, mipBias).w;
#else
	float alpha = normalAlpha.SampleBias(samplerLinear, input.texCoord, -1.0f).w;
#endif
	
	if (alpha < 0.5f)
		discard;
}
