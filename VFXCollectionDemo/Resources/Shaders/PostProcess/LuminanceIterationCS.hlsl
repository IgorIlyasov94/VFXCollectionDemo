static const uint NUM_THREADS = 64;

cbuffer RootConstants : register(b0)
{
	uint area;
};

struct Input
{
	uint3 dispatchThreadId : SV_DispatchThreadID;
	uint3 groupId : SV_GroupID;
	uint groupIndex : SV_GroupIndex;
};

RWStructuredBuffer<float> luminanceBuffer : register(u0);

groupshared float groupBuffer[NUM_THREADS];

[numthreads(NUM_THREADS, 1, 1)]
void main(Input input)
{
	if (input.dispatchThreadId.x < area)
		groupBuffer[input.groupIndex] = luminanceBuffer[input.dispatchThreadId.x];
	else
		groupBuffer[input.groupIndex] = 0.0f;
	
	GroupMemoryBarrierWithGroupSync();
	
	if (input.groupIndex < 32)
		groupBuffer[input.groupIndex] += groupBuffer[input.groupIndex + 32];
	
	GroupMemoryBarrierWithGroupSync();
	
	if (input.groupIndex < 16)
		groupBuffer[input.groupIndex] += groupBuffer[input.groupIndex + 16];
	
	GroupMemoryBarrierWithGroupSync();
	
	if (input.groupIndex < 8)
		groupBuffer[input.groupIndex] += groupBuffer[input.groupIndex + 8];
	
	GroupMemoryBarrierWithGroupSync();
	
	if (input.groupIndex < 4)
	{
		float result = groupBuffer[0];
		result += groupBuffer[1];
		result += groupBuffer[2];
		result += groupBuffer[3];
		result += groupBuffer[4];
		result += groupBuffer[5];
		result += groupBuffer[6];
		result += groupBuffer[7];
		
		luminanceBuffer[input.groupId.x] = result;
	}
}
