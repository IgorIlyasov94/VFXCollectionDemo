cbuffer RootConstants : register(b0)
{
	float4x4 world;
	uint lightMatrixStartIndex;
};

cbuffer LightConstants : register(b1)
{
	float4x4 lightViewProjection[LIGHT_MATRICES_NUMBER];
};

struct Input
{
	float4 position : POSITION;
};

struct Output
{
	float4 position : SV_Position;
	uint targetIndex : SV_RenderTargetArrayIndex;
};

[maxvertexcount(18)]
void main(triangle Input input[3], inout TriangleStream<Output> outputStream)
{
	[unroll]
	for (uint faceIndex = 0; faceIndex < 6; faceIndex++)
	{
		Output output = (Output)0;
		output.targetIndex = faceIndex;
		
		uint lightViewProjectionIndex = lightMatrixStartIndex + faceIndex;
		
		[unroll]
		for (uint vertexIndex = 0; vertexIndex < 3; vertexIndex++)
		{
			output.position = mul(lightViewProjection[lightViewProjectionIndex], input[vertexIndex].position);
			outputStream.Append(output);
		}
		
		outputStream.RestartStrip();
	}
}
