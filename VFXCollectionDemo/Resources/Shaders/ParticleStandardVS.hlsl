struct Particle
{
	float3 position;
	float rotation;
	
	float3 velocity;
	float rotationSpeed;
	
	float2 size;
	float life;
	float startLife;
};

cbuffer MutableConstants : register(b0)
{
	float4x4 invView;
	float4x4 viewProjection;
	
	float2 atlasElementOffset;
	float2 atlasElementSize;
	
	float colorIntensity;
	float3 perlinNoiseTiling;
	
	float3 perlinNoiseScrolling;
	float particleTurbulence;
	
	float time;
	float3 padding;
};

struct Input
{
	float3 position : POSITION;
	float2 texCoord : TEXCOORD0;
	uint instanceId : SV_InstanceID;
};

struct Output
{
	float4 position : SV_Position;
	float4 color : COLOR0;
	float2 texCoord : TEXCOORD0;
};

StructuredBuffer<Particle> particleBuffer : register(t0);
Texture2D vfxAnimation : register(t1);
Texture2D perlinNoise : register(t2);

SamplerState samplerLinear : register(s0);

float2 RotateQuad(float2 position, float angle)
{
	float sinAngle;
	float cosAngle;
	sincos(angle, sinAngle, cosAngle);
	
	float2x2 rotationMatrix =
	{
		cosAngle, -sinAngle,
		sinAngle, cosAngle
	};
	
	return mul(rotationMatrix, position);
}

Output main(Input input)
{
	Output output = (Output)0;
	
	Particle particle = particleBuffer[input.instanceId];
	
	float alpha = particle.life / (float)particle.startLife;
	
	float4 sizeData = vfxAnimation.SampleLevel(samplerLinear, float2(alpha, 0.9f), 0.0f);
	
	float2 size = sizeData.xy * sizeData.w;
	size *= particle.size;
	
	float3 localPosition = float3(input.position.xy * size, 0.0f);
	localPosition.xy = RotateQuad(localPosition.xy, particle.rotation);
	
	float4 worldPosition = float4(mul((float3x3)invView, localPosition), 1.0f);
	
	float2 texCoordXY = particle.position.xy * perlinNoiseTiling.xy + frac(time * perlinNoiseScrolling.xy);
	float2 texCoordYZ = particle.position.yz * perlinNoiseTiling.yz + frac(time * perlinNoiseScrolling.yz);
	float2 texCoordZX = particle.position.zx * perlinNoiseTiling.zx + frac(time * perlinNoiseScrolling.zx);
	
	float3 displacement = perlinNoise.SampleLevel(samplerLinear, texCoordXY, 0.0f).xyz;
	displacement += perlinNoise.SampleLevel(samplerLinear, texCoordYZ, 0.0f).xyz;
	displacement += perlinNoise.SampleLevel(samplerLinear, texCoordZX, 0.0f).xyz;
	
	displacement = (displacement * 2.0f / 3.0f - 1.0f.xxx) * particleTurbulence;
	
	worldPosition.xyz += particle.position + displacement;
	
	output.position = mul(viewProjection, worldPosition);
	output.color = vfxAnimation.SampleLevel(samplerLinear, float2(alpha, 0.1f), 0.0f);
	output.texCoord = input.texCoord * atlasElementSize + atlasElementOffset;
	
	return output;
}
