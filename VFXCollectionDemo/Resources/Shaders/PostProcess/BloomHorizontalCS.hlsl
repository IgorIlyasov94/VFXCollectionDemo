static const uint NUM_THREADS = 64;

static const int SAMPLES_NUMBER = 17;
static const int HALF_SAMPLES_NUMBER = 8;
static const int TEXTURE_BLOCK_WIDTH = NUM_THREADS - HALF_SAMPLES_NUMBER * 2;

static const float PI = 3.14159265359f;
static const float NORMAL_DISTRIBUTION_SIGMA = 3.0f;

cbuffer RootConstants : register(b0)
{
	uint width;
	uint area;
	uint halfWidth;
	uint quartArea;
	float middleGray;
	float whiteCutoff;
	float brightThreshold;
};

struct Input
{
	uint3 dispatchThreadId : SV_DispatchThreadID;
	uint3 groupId : SV_GroupID;
	uint groupIndex : SV_GroupIndex;
};

Texture2D<float4> sceneColor : register(t0);

StructuredBuffer<float> luminanceBuffer : register(t1);

RWStructuredBuffer<float4> bloomBuffer : register(u0);

groupshared float3 groupBuffer[NUM_THREADS];

float Weight(int x, float sigma)
{
	return exp(-x * x / (2.0f * sigma * sigma)) / (sigma * sqrt(2.0f * PI));
}

float3 ToneMapping(float3 color, float luminance)
{
	color *= (1.0f.xxx + luminance / (whiteCutoff * whiteCutoff)) / (1.0f.xxx + luminance);
	color = pow(color, 1.0f / 2.2f);
	
	return color;
}

[numthreads(NUM_THREADS, 1, 1)]
void main(Input input)
{
	float luminance = luminanceBuffer[0] / area;
	
	uint bufferIndex = max(input.groupIndex + TEXTURE_BLOCK_WIDTH * input.groupId.x - HALF_SAMPLES_NUMBER, 0);
	uint2 texCoord;
	texCoord.x = bufferIndex % halfWidth;
	texCoord.y = bufferIndex / halfWidth;
	
	bool outOfBounds = texCoord.x < HALF_SAMPLES_NUMBER || texCoord.x > (halfWidth - HALF_SAMPLES_NUMBER);
	
	texCoord *= 2u;
	int3 texCoordI = int3(texCoord.x, texCoord.y, 0);
	
	float3 color = sceneColor.Load(texCoordI).xyz;
	color += sceneColor.Load(texCoordI + int3(0, 1, 0)).xyz;
	color += sceneColor.Load(texCoordI + int3(1, 0, 0)).xyz;
	color += sceneColor.Load(texCoordI + int3(1, 1, 0)).xyz;
	
	color = max(color * 0.25f - brightThreshold, 0.0f.xxx);
	color = ToneMapping(color, luminance);
	
	groupBuffer[input.groupIndex] = outOfBounds ? 0.0f.xxx : color;
	
	GroupMemoryBarrierWithGroupSync();
	
	bool isValidRange = input.groupIndex >= (uint)HALF_SAMPLES_NUMBER;
	isValidRange = isValidRange && (input.groupIndex < (NUM_THREADS - HALF_SAMPLES_NUMBER));
	isValidRange = isValidRange && (bufferIndex < quartArea);
	
	if (!isValidRange)
		return;
	
	float3 resultColor = 0.0f.xxx;
	
	[unroll]
	for (int sampleIndex = -HALF_SAMPLES_NUMBER; sampleIndex <= HALF_SAMPLES_NUMBER; sampleIndex++)
		resultColor += groupBuffer[input.groupIndex + sampleIndex] * Weight(sampleIndex, NORMAL_DISTRIBUTION_SIGMA);
	
	bloomBuffer[bufferIndex] = float4(resultColor, 1.0f);
}
