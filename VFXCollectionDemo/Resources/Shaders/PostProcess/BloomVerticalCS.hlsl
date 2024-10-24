static const uint NUM_THREADS = 64;

static const int SAMPLES_NUMBER = 17;
static const int HALF_SAMPLES_NUMBER = 8;
static const int TEXTURE_BLOCK_WIDTH = NUM_THREADS - HALF_SAMPLES_NUMBER * 2;

static const float PI = 3.14159265359f;
static const float NORMAL_DISTRIBUTION_SIGMA = 3.0f;

cbuffer RootConstants : register(b0)
{
	uint width;
	uint height;
	uint area;
};

struct Input
{
	uint3 dispatchThreadId : SV_DispatchThreadID;
	uint3 groupId : SV_GroupID;
	uint groupIndex : SV_GroupIndex;
};

RWStructuredBuffer<float4> bloomBuffer : register(u0);

groupshared float3 groupBuffer[NUM_THREADS];

float Weight(int x, float sigma)
{
	return exp(-x * x / (2.0f * sigma * sigma)) / (sigma * sqrt(2.0f * PI));
}

[numthreads(NUM_THREADS, 1, 1)]
void main(Input input)
{
	uint bufferIndex = max(input.groupIndex + TEXTURE_BLOCK_WIDTH * input.groupId.x - HALF_SAMPLES_NUMBER, 0);
	
	uint2 texCoord;
	texCoord.x = bufferIndex / height;
	texCoord.y = bufferIndex % height;
	
	bool outOfBounds = texCoord.y < HALF_SAMPLES_NUMBER || texCoord.y > (height - HALF_SAMPLES_NUMBER);
	
	bufferIndex = texCoord.x + texCoord.y * width;
	
	float3 color = bloomBuffer[clamp(bufferIndex, 0, area - 1)].xyz;
	groupBuffer[input.groupIndex] = outOfBounds ? 0.0f.xxx : color;
	
	GroupMemoryBarrierWithGroupSync();
	
	bool isValidRange = input.groupIndex >= (uint)HALF_SAMPLES_NUMBER;
	isValidRange = isValidRange && (input.groupIndex < (NUM_THREADS - HALF_SAMPLES_NUMBER));
	isValidRange = isValidRange && (bufferIndex < area);
	
	if (!isValidRange)
		return;
	
	float3 resultColor = 0.0f.xxx;
	
	[unroll]
	for (int sampleIndex = -HALF_SAMPLES_NUMBER; sampleIndex <= HALF_SAMPLES_NUMBER; sampleIndex++)
		resultColor += groupBuffer[input.groupIndex + sampleIndex] * Weight(sampleIndex, NORMAL_DISTRIBUTION_SIGMA);
	
	bloomBuffer[bufferIndex] = float4(resultColor, 1.0f);
}
