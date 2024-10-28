struct Vegetation
{
	float4x4 world;
	float2 atlasElementOffset;
	float tiltAmplitude;
	float height;
};

cbuffer RootConstants : register(b0)
{
	uint lightMatrixStartIndex;
};

cbuffer MutableConstants : register(b1)
{
	float4x4 viewProjection;
	
	float3 cameraPosition;
	float time;
	
	float2 atlasElementSize;
	float2 perlinNoiseTiling;
	
	float3 windDirection;
	float windStrength;
};

cbuffer LightConstants : register(b2)
{
	float4x4 lightViewProjection[LIGHT_MATRICES_NUMBER];
};

struct Input
{
	float4 position : POSITION;
	float2 texCoord : TEXCOORD0;
};

struct Output
{
	float4 position : SV_Position;
	uint targetIndex : SV_RenderTargetArrayIndex;
	float2 texCoord : TEXCOORD0;
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
			output.texCoord = input[vertexIndex].texCoord;
			outputStream.Append(output);
		}
		
		outputStream.RestartStrip();
	}
}
