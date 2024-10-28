static const uint NUM_THREADS = 64;

static const int SAMPLES_NUMBER = 8;

static const float PI = 3.14159265359f;
static const float NORMAL_DISTRIBUTION_SIGMA = 2.75f;

static const float3 STRENGTH_COEFF = float3(0.4f, 0.6f, 1.0f);

cbuffer RootConstants : register(b0)
{
	uint widthU;
	uint area;
	float width;
	float height;
};

Texture2D sceneColor : register(t0);
Texture2D<float2> sceneMotion : register(t1);

RWStructuredBuffer<float4> sceneBuffer : register(u0);

SamplerState samplerLinear : register(s0);

float Weight(int x, float sigma)
{
	return 2.0f * exp(-x * x / (2.0f * sigma * sigma)) / (sigma * sqrt(2.0f * PI));
}

[numthreads(NUM_THREADS, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint bufferIndex = dispatchThreadId.x;
	
	if (bufferIndex >= area)
		return;
	
	float2 texCoord;
	texCoord.x = (bufferIndex % widthU) / width;
	texCoord.y = (bufferIndex / widthU) / height;
	
	float2 motion = sceneMotion.SampleLevel(samplerLinear, saturate(texCoord), 0.0f).xy;
	float2 motionR = motion * STRENGTH_COEFF.x;
	float2 motionG = motion * STRENGTH_COEFF.y;
	float2 motionB = motion * STRENGTH_COEFF.z;
	
	float4 color = 0.0f.xxxx;
	
	[unroll]
	for (int sampleIndex = 0; sampleIndex < SAMPLES_NUMBER; sampleIndex++)
	{
		float t = saturate(sampleIndex / (float)(SAMPLES_NUMBER - 1)) * 5.0f;
		
		float2 offsetR = saturate(texCoord + motionR * t);
		float2 offsetG = saturate(texCoord + motionG * t);
		float2 offsetB = saturate(texCoord + motionB * t);
		
		float2 colorRA = sceneColor.SampleLevel(samplerLinear, offsetR, 0.0f).xw;
		float colorG = sceneColor.SampleLevel(samplerLinear, offsetG, 0.0f).y;
		float colorB = sceneColor.SampleLevel(samplerLinear, offsetB, 0.0f).z;
		
		color += float4(colorRA.x, colorG, colorB, colorRA.y) * Weight(sampleIndex, NORMAL_DISTRIBUTION_SIGMA);
	}
	
	sceneBuffer[bufferIndex] = color;
}
