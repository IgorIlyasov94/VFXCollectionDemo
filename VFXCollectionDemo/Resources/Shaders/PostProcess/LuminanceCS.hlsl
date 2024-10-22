static const uint NUM_THREADS = 64;
static const float3 LUMINANCE_VECTOR = float3(0.2126f, 0.7152f, 0.0722f);

cbuffer RootConstants : register(b0)
{
	uint width;
	uint area;
};

struct Input
{
	uint3 dispatchThreadId : SV_DispatchThreadID;
	uint3 groupId : SV_GroupID;
	uint groupIndex : SV_GroupIndex;
};

#if defined(MOTION_BLUR) || defined(VOLUMETRIC_FOG)
StructuredBuffer<float4> sceneBuffer : register(t0);
#else
Texture2D sceneColor : register(t0);
#endif

RWStructuredBuffer<float> luminanceBuffer : register(u0);

groupshared float groupBuffer[NUM_THREADS];

[numthreads(NUM_THREADS, 1, 1)]
void main(Input input)
{
	if (input.dispatchThreadId.x < area)
	{
		uint bufferIndex = min(input.dispatchThreadId.x, area - 1);
		
#if defined(MOTION_BLUR) || defined(VOLUMETRIC_FOG)
		groupBuffer[input.groupIndex] = dot(sceneBuffer[bufferIndex].xyz, LUMINANCE_VECTOR);
#else
		int3 texCoord = int3(bufferIndex % width, bufferIndex / width, 0);
		
		groupBuffer[input.groupIndex] = dot(sceneColor.Load(texCoord).xyz, LUMINANCE_VECTOR);
#endif
	}
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
