static const uint NUM_THREADS = 64;
static const uint MAX_FORCES_NUMBER = 4;

static const float PI = 3.14159265f;

static const uint FORCE_TYPE_ATTRACTOR = 0;
static const uint FORCE_TYPE_CIRCULAR = 1;

struct ParticleSystemForce
{
	float3 position;
	float strength;
	float3 axis;
	uint type;
	float nAccelerationCoeff;
	float tAccelerationCoeff;
	float2 padding;
};

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
	float3 emitterOrigin;
	float padding;
	
	float3 minParticleVelocity;
	float particleDamping;
	
	float3 maxParticleVelocity;
	float particleTurbulence;
	
	float minRotation;
	float maxRotation;
	float minRotationSpeed;
	float maxRotationSpeed;
	
	float2 minSize;
	float2 maxSize;
	
	float minLifeSec;
	float maxLifeSec;
	float time;
	float deltaTime;
	
	uint averageParticleEmit;
	uint maxParticlesNumber;
	float2 perlinNoiseSize;
	
	float4 random0;
	float4 random1;
	
	float maxLightIntensity;
	float3 emitterRadius;
	
	float lightRange;
	float3 emitterRadiusOffset;
	
	ParticleSystemForce forces[MAX_FORCES_NUMBER];
};

Texture2D perlinNoise : register(t0);

RWStructuredBuffer<Particle> particleBuffer : register(u0);

float3 SumAccelerations(float3 particlePosition)
{
	float3 result = 0.0f.xxx;
	
	[unroll]
	for (uint forceIndex = 0; forceIndex < MAX_FORCES_NUMBER; forceIndex++)
	{
		ParticleSystemForce force = forces[forceIndex];
		
		float3 acceleration = force.position - particlePosition;
		
		float3 accelerationN = normalize(any(acceleration) ? acceleration : float3(0.0f, 0.0f, 1.0f));
		float3 accelerationT = (force.type == FORCE_TYPE_CIRCULAR) ?
			cross(accelerationN, force.axis) :
			accelerationN;
		
		float3 accelerationDir = accelerationN * force.nAccelerationCoeff;
		accelerationDir += accelerationT * force.tAccelerationCoeff;
		accelerationDir = normalize(accelerationDir);
		
		acceleration = length(acceleration) * accelerationDir;
		
		float damping = dot(acceleration, acceleration) + 0.001f;
		damping = rsqrt(damping);
		damping *= damping * damping;
		
		acceleration *= force.strength * damping;
		
		result += acceleration;
	}
	
	return result;
}

float3 RandomPosition(float3 random, float3 offset, float3 radius)
{
	float3 shiftedRandom = random - 0.5f.xxx;
	float3 randomDirection = normalize(shiftedRandom);
	float3 randomRadius = abs(shiftedRandom * 2.0f) * radius;
	
	return (randomRadius + offset) * randomDirection;
}

Particle EmitParticle(float4 random0_0, float4 random1_0, float4 random2_0)
{
	Particle newParticle = (Particle)0;
	newParticle.position = emitterOrigin + RandomPosition(random0_0.xyz, emitterRadiusOffset, emitterRadius);
	newParticle.velocity = lerp(minParticleVelocity, maxParticleVelocity, random1_0.xyz);
	
	newParticle.rotation = lerp(minRotation, maxRotation, random2_0.z);
	newParticle.rotationSpeed = lerp(minRotationSpeed, maxRotationSpeed, random2_0.x);
	
	newParticle.size = lerp(minSize, maxSize, random2_0.y);
	newParticle.startLife = lerp(minLifeSec, maxLifeSec, random1_0.w);
	
	newParticle.life = newParticle.startLife;
	
	return newParticle;
}

[numthreads(NUM_THREADS, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint particleIndex = dispatchThreadId.x;
	
	if (particleIndex >= maxParticlesNumber)
		return;
	
	Particle particle = particleBuffer[particleIndex];
	
	float2 swizzledCoord = ((float)(particleIndex) + time) / maxParticlesNumber;
	uint2 texCoord0 = (uint2)(frac(particle.position.xy + swizzledCoord) * perlinNoiseSize);
	uint2 texCoord1 = (uint2)(frac(particle.position.zx + swizzledCoord) * perlinNoiseSize.yx);
	
	float4 randomModifier0 = perlinNoise[texCoord1];
	float4 randomModifier1 = perlinNoise[texCoord0];
	float4 random0_0 = frac(random0 + randomModifier0 + randomModifier1);
	float4 random1_0 = frac(random1 + randomModifier0 + randomModifier1);
	float4 random2_0 = frac(float4(random0.zw, random1.xy) + randomModifier0.wzxy + randomModifier1.yxwz);
	
	[branch]
	if (particle.life <= 0.0f)
	{
		float emitProbability = averageParticleEmit * deltaTime;
		
		if (random0_0.w <= emitProbability)
			particle = EmitParticle(random0_0, random1_0, random2_0);
	}
	else
	{
		float3 velocity = particle.velocity;
		float3 acceleration = SumAccelerations(particle.position);
		
		particle.position += velocity * deltaTime;
		particle.velocity += acceleration * deltaTime * deltaTime;
		particle.velocity *= particleDamping;
		
		particle.rotation += particle.rotationSpeed * deltaTime;
		particle.rotationSpeed *= particleDamping;
		
		particle.life -= deltaTime;
	}
	
	particleBuffer[particleIndex] = particle;
}
