static const uint NUM_THREADS = 64;

static const int SAMPLES_NUMBER = 8;

static const float PI = 3.14159265359f;
static const float NORMAL_DISTRIBUTION_SIGMA = 2.75f;

static const float3 STRENGTH_COEFF = float3(0.2f, 0.6f, 1.0f);

cbuffer RootConstants : register(b0)
{
	uint widthU;
	uint area;
	float width;
	float height;
	float threshold;
};

#if defined(FSR) || defined(VOLUMETRIC_FOG)
Texture2D<float2> sceneMotion : register(t0);
#else
Texture2D sceneColor : register(t0);
Texture2D<float2> sceneMotion : register(t1);
#endif

RWTexture2D<float4> sceneTarget : register(u0);

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
	
	uint2 texCoordU = uint2(bufferIndex % widthU, bufferIndex / widthU);
	
	float2 texCoord;
	texCoord.x = texCoordU.x / width;
	texCoord.y = texCoordU.y / height;
	
	float2 motion = sceneMotion.SampleLevel(samplerLinear, saturate(texCoord), 0.0f).xy;
	float2 motionR = motion * STRENGTH_COEFF.x;
	float2 motionG = motion * STRENGTH_COEFF.y;
	float2 motionB = motion * STRENGTH_COEFF.z;
	
#if defined(FSR) || defined(VOLUMETRIC_FOG)
		float4 originColor = sceneTarget.Load(texCoordU);
#else
		float4 originColor = sceneColor.SampleLevel(samplerLinear, texCoord, 0.0f);
#endif
	
	const float firstWeight = Weight(0, NORMAL_DISTRIBUTION_SIGMA);
	
	float4 color = 0.0f.xxxx;
	
	[unroll]
	for (int sampleIndex = 1; sampleIndex < SAMPLES_NUMBER; sampleIndex++)
	{
		float t = saturate(sampleIndex / (float)(SAMPLES_NUMBER - 1)) * 5.0f;
		
		float2 offsetR = saturate(texCoord + motionR * t);
		float2 offsetG = saturate(texCoord + motionG * t);
		float2 offsetB = saturate(texCoord + motionB * t);
		
#if defined(FSR) || defined(VOLUMETRIC_FOG)
		uint2 sizeVector = uint2(width, height);
		
		uint2 offsetRU = (uint2)round(offsetR * sizeVector);
		uint2 offsetGU = (uint2)round(offsetG * sizeVector);
		uint2 offsetBU = (uint2)round(offsetB * sizeVector);
		
		float2 colorRA = sceneTarget.Load(offsetRU).xw;
		float colorG = sceneTarget.Load(offsetGU).y;
		float colorB = sceneTarget.Load(offsetBU).z;
#else
		float2 colorRA = sceneColor.SampleLevel(samplerLinear, offsetR, 0.0f).xw;
		float colorG = sceneColor.SampleLevel(samplerLinear, offsetG, 0.0f).y;
		float colorB = sceneColor.SampleLevel(samplerLinear, offsetB, 0.0f).z;
#endif
		
		color += float4(colorRA.x, colorG, colorB, colorRA.y) * Weight(sampleIndex, NORMAL_DISTRIBUTION_SIGMA);
	}
	
	color = (length(motion) >= threshold) ? color + originColor * firstWeight : originColor;
	
	sceneTarget[texCoordU] = color;
}
