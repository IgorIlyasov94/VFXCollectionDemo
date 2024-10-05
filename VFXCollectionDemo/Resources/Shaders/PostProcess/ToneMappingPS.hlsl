static const float3 LUMINANCE_VECTOR = float3(0.2126f, 0.7152f, 0.0722f);

cbuffer RootConstants : register(b0)
{
	uint width;
	uint area;
	uint halfWidth;
	uint quartArea;
	float middleGray;
	float whiteCutoff;
	float brightThreshold;
	float bloomIntensity;
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

float3 ToneMapping(float3 color, float luminance)
{
	color *= (1.0f.xxx + luminance / (whiteCutoff * whiteCutoff)) / (1.0f.xxx + luminance);
	color = pow(color, 1.0f / 2.2f);
	
	return color;
}

Output main(Input input)
{
	Output output = (Output)0;
	
	uint maxCoordValue = area - 1;
	
	uint2 bufferCoord = (uint2)input.position.xy;
	uint bufferIndex = min(bufferCoord.x + bufferCoord.y * width, maxCoordValue);
	
	float3 color = sceneBuffer[bufferIndex].xyz;
	
	float luminance = luminanceBuffer[0u] / (float)area;
	
	float gray = dot(color, LUMINANCE_VECTOR);
	float3 rodColor = float3(1.0f, 0.9f, 1.5f);
	float colorShiftFactor = 0.75f;
	
	color = lerp(gray * rodColor, color, colorShiftFactor);
	color = ToneMapping(color, luminance);
	
	maxCoordValue = quartArea - 1;
	
	bufferCoord = bufferCoord / 2u;
	bufferIndex = min(bufferCoord.x + bufferCoord.y * halfWidth, maxCoordValue);
	
	float3 bloom = bloomBuffer[bufferIndex].xyz;
	
	output.color = float4(saturate(bloom * bloomIntensity + color), 1.0f);
	
	return output;
}
