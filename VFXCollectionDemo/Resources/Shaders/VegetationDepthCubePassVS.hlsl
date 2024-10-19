struct Vegetation
{
	float4x4 world;
	float2 atlasElementOffset;
	float windInfluence;
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
	float4 position : POSITION;
	float2 texCoord : TEXCOORD0;
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
	
	float2 noiseXZTexCoord = vegetation.world[3].xz * perlinNoiseTiling;
	float2 noiseYZTexCoord = vegetation.world[3].yz * perlinNoiseTiling;
	
	float noiseXZ = perlinNoise.SampleLevel(samplerLinear, noiseXZTexCoord, 0.0f).x;
	float noiseYZ = perlinNoise.SampleLevel(samplerLinear, noiseYZTexCoord, 0.0f).x;
	float noise = (noiseXZ + noiseYZ) * 0.25f + 0.5f;
	
	float t = saturate(sin(time * windStrength * noise) * 0.5f + 0.5f);
	
	float height = abs(vegetation.height);
	bool isCap = vegetation.height < 0.0f;
	
	float3 shift = windDirection;
	shift.z -= 0.5f * height * dot(windDirection, windDirection);
	shift = lerp(shift * vegetation.windInfluence, 0.0f.xxx, t);
	float shiftCoeff = (1.0f - (isCap ? 0.5f : input.texCoord.y)) * height;
	
	worldPosition.xyz += shift * shiftCoeff;
	
	output.position = worldPosition;
	output.texCoord = input.texCoord * atlasElementSize + vegetation.atlasElementOffset;
	
	return output;
}
