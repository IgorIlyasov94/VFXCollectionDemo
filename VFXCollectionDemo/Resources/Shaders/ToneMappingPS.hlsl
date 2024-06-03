static const float3 LUMINANCE_VECTOR = float3(0.2126f, 0.7152f, 0.0722f);

cbuffer RootConstants : register(b0)
{
	uint width;
	uint area;
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

Texture2D sceneColor : register(t0);
StructuredBuffer<float> luminanceBuffer : register(t1);
StructuredBuffer<float4> bloomBuffer : register(t2);

SamplerState samplerLinear : register(s0);

Output main(Input input)
{
	Output output = (Output)0;
	
	float3 color = sceneColor.SampleLevel(samplerLinear, input.texCoord, 0.0f).xyz;
	
	float luminance = luminanceBuffer[0u] / area;
	
	float gray = dot(color, LUMINANCE_VECTOR);
	float3 rodColor = float3(0.95f, 1.1f, 1.4f);
	float colorShiftFactor = saturate(luminance / 4.0f);
	
	//color = lerp(gray * rodColor, color, colorShiftFactor);
	
	color *= middleGray / (luminance + 0.001f);
	color *= 1.0f + color / whiteCutoff;
	color /= 1.0f + color;
	
	uint2 bufferCoord = ((uint2)input.position.xy) / 2u;
	
	float3 bloom = bloomBuffer[clamp(bufferCoord.x + bufferCoord.y * width, 0, area)].xyz;
	
	output.color = float4(saturate(color + bloom), 1.0f);
	
	return output;
}
