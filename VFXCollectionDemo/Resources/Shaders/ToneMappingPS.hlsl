static const float3 LUMINANCE_VECTOR = float3(0.2126f, 0.7152f, 0.0722f);

cbuffer RootConstants : register(b0)
{
	uint width;
	uint area;
	uint halfWidth;
	uint quartArea;
	float middleGray;
	float whiteCutoff;
};

struct Input
{
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD0;
};

struct Output
{
	float4 color : SV_TARGET0;
};

StructuredBuffer<float4> sceneBuffer : register(t0);
StructuredBuffer<float> luminanceBuffer : register(t1);
StructuredBuffer<float4> bloomBuffer : register(t2);

Output main(Input input)
{
	Output output = (Output)0;
	
	uint maxCoordValue = area - 1;
	
	uint2 bufferCoord = (uint2)input.position.xy;
	uint bufferIndex = min(bufferCoord.x + bufferCoord.y * width, maxCoordValue);
	
	float3 color = sceneBuffer[bufferIndex].xyz;
	
	float luminance = luminanceBuffer[0u] / area;
	
	float gray = dot(color, LUMINANCE_VECTOR);
	float3 rodColor = float3(0.95f, 1.1f, 1.4f);
	float colorShiftFactor = saturate(luminance / 4.0f);
	
	//color = lerp(gray * rodColor, color, colorShiftFactor);
	
	color *= middleGray / (luminance + 0.001f);
	color *= 1.0f + color / whiteCutoff;
	color /= 1.0f + color;
	
	maxCoordValue = quartArea - 1;
	
	bufferCoord = bufferCoord / 2u;
	bufferIndex = min(bufferCoord.x + bufferCoord.y * halfWidth, maxCoordValue);
	
	float3 bloom = bloomBuffer[bufferIndex].xyz;
	
	output.color = float4(saturate(color + bloom), 1.0f);
	
	return output;
}
