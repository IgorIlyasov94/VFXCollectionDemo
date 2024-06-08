static const uint NUM_THREADS = 64;
static const float3 STRENGTH_COEFF = float3(1.0f, 0.85f, 0.7f);

cbuffer RootConstants : register(b0)
{
	uint widthU;
	uint area;
	float width;
	float height;
};

Texture2D sceneColor : register(t0);
Texture2D<float2> sceneDistortion : register(t1);

RWStructuredBuffer<float4> sceneBuffer : register(u0);

SamplerState samplerLinear : register(s0);

[numthreads(NUM_THREADS, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint bufferIndex = dispatchThreadId.x;
	
	if (bufferIndex >= area)
		return;
	
	float2 texCoord;
	texCoord.x = (bufferIndex % widthU) / width;
	texCoord.y = (bufferIndex / widthU) / height;
	
	float2 distortion = sceneDistortion.SampleLevel(samplerLinear, saturate(texCoord), 0.0f).xy;
	float2 distortionR = distortion * STRENGTH_COEFF.x;
	float2 distortionG = distortion * STRENGTH_COEFF.y;
	float2 distortionB = distortion * STRENGTH_COEFF.z;
	
	float colorR = sceneColor.SampleLevel(samplerLinear, saturate(texCoord) + distortionR, 0.0f).x;
	float colorG = sceneColor.SampleLevel(samplerLinear, saturate(texCoord) + distortionG, 0.0f).y;
	float colorB = sceneColor.SampleLevel(samplerLinear, saturate(texCoord) + distortionB, 0.0f).z;
	
	colorR += pow(length(distortionR), 2.0f) * 3.2f;
	colorG += pow(length(distortionG), 2.0f) * 6.2f;
	colorB += pow(length(distortionB), 2.0f) * 16.2f;
	
	sceneBuffer[bufferIndex] = float4(colorR, colorG, colorB, 1.0f);
}
