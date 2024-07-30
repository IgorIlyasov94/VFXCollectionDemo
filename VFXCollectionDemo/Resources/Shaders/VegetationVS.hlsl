struct Vegetation
{
	float4x4 world;
	float2 atlasElementOffset;
	float tiltAmplitude;
	float height;
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
	
	float zNear;
	float zFar;
	float2 padding;
};

struct Input
{
	float3 position : POSITION;
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
	float2 texCoord : TEXCOORD0;
	uint instanceId : SV_InstanceID;
};

struct Output
{
	float4 position : SV_Position;
	float3 normal : NORMAL;
	float3 binormal : BINORMAL;
	float3 tangent : TANGENT;
	float2 texCoord : TEXCOORD0;
	float3 worldPosition : TEXCOORD1;
};

StructuredBuffer<Vegetation> vegetationBuffer : register(t0);
Texture2D perlinNoise : register(t1);

SamplerState samplerLinear : register(s0);

Output main(Input input)
{
	Output output = (Output)0;
	
	Vegetation vegetation = vegetationBuffer[input.instanceId];
	
	float4 localPosition = float4(input.position, 1.0f);
	float4 worldPosition = mul(vegetation.world, localPosition);
	
	float2 noiseXZTexCoord = worldPosition.xz * perlinNoiseTiling;
	float2 noiseYZTexCoord = worldPosition.yz * perlinNoiseTiling;
	
	float noiseXZ = perlinNoise.SampleLevel(samplerLinear, noiseXZTexCoord, 0.0f).x;
	float noiseYZ = perlinNoise.SampleLevel(samplerLinear, noiseYZTexCoord, 0.0f).x;
	float noise = (noiseXZ + noiseYZ) * 0.25f + 0.5f;
	
	float t = saturate(sin(time * windStrength * noise) * 0.5f + 0.5f);
	
	float height = abs(vegetation.height);
	bool isCap = vegetation.height < 0.0f;
	
	float3 shift = min(windDirection, vegetation.tiltAmplitude.xxx) + float3(0.0f, 0.0f, 0.0001f);
	shift.z -= 0.5f * height * dot(windDirection, windDirection);
	shift = lerp(normalize(shift), 0.0f.xxx, t);
	float shiftCoeff = (1.0f - (isCap ? 0.0f : input.texCoord.y)) * height;
	
	worldPosition.xyz += shift * shiftCoeff;
	
	output.position = mul(viewProjection, worldPosition);
	
	float3 normal = normalize(mul((float3x3)vegetation.world, input.normal.xyz));
	float3 tangent = normalize(mul((float3x3)vegetation.world, input.tangent.xyz));
	
	output.normal = isCap ? float3(0.0f, 0.0f, 1.0f) : normal;
	output.tangent = isCap ? float3(0.0f, -1.0f, 0.0f) : tangent;
	
	output.binormal = cross(output.tangent, output.normal);
	
	output.texCoord = input.texCoord * atlasElementSize + vegetation.atlasElementOffset;
	output.worldPosition = worldPosition.xyz;
	
	return output;
}
