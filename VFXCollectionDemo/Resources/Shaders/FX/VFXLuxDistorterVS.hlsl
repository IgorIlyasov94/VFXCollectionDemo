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
	
	float2 noiseTiling;
	float2 noiseScrollSpeed;
	
	float time;
	float deltaTime;
	float particleNumber;
	float noiseStrength;
	
	float velocityStrength;
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
	float4 texCoord : TEXCOORD0;
	float3 velocityAlpha : TEXCOORD1;
};

StructuredBuffer<Particle> particleBuffer : register(t0);
Texture2D vfxAnimation : register(t1);

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
	
	float2 size = vfxAnimation.SampleLevel(samplerLinear, float2(alpha, 0.9f), 0.0f).xy;
	size *= particle.size;
	
	float3 localPosition = float3(input.position.xy * size, 0.0f);
	localPosition.xy = RotateQuad(localPosition.xy, particle.rotation);
	
	float4 worldPosition = float4(mul((float3x3)invView, localPosition), 1.0f);
	worldPosition.xyz += particle.position;
	
	output.position = mul(viewProjection, worldPosition);
	output.texCoord.xy = input.texCoord * atlasElementSize + atlasElementOffset;
	
	float swizzledOffset = input.instanceId / particleNumber;
	
	output.texCoord.zw = input.texCoord * noiseTiling + frac((time + swizzledOffset) * noiseScrollSpeed);
	
	float4 lastPosition = worldPosition;
	lastPosition.xyz -= particle.velocity * deltaTime;
	lastPosition = mul(viewProjection, lastPosition);
	
	float2 projPosition = output.position.xy / output.position.w;
	float2 lastProjPosition = lastPosition.xy / lastPosition.w;
	
	output.velocityAlpha.xy = (projPosition - lastProjPosition) * 0.5f;
	output.velocityAlpha.y = -output.velocityAlpha.y;
	output.velocityAlpha.z = vfxAnimation.SampleLevel(samplerLinear, float2(alpha, 0.1f), 0.0f).w;
	
	return output;
}
