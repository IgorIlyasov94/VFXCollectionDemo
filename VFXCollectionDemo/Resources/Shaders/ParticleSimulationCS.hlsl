static const uint NUM_THREADS = 64;
static const uint MAX_ATTRACTORS_NUMBER = 8;

struct ParticleSystemAttractor
{
	float3 position;
	float strength;
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
	float emitterRadius;
	
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
	
	ParticleSystemAttractor attractors[MAX_ATTRACTORS_NUMBER];
};

Texture2D perlinNoise : register(t0);

RWStructuredBuffer<Particle> particleBuffer : register(u0);

float3 SumAccelerations(float3 particlePosition)
{
	float3 result = 0.0f.xxx;
	
	[unroll]
	for (uint attractorIndex = 0; attractorIndex < MAX_ATTRACTORS_NUMBER; attractorIndex++)
	{
		ParticleSystemAttractor attractor = attractors[attractorIndex];
		
		float3 acceleration = attractor.position - particlePosition;
		
		float damping = dot(acceleration, acceleration) + 0.001f;
		damping = rsqrt(damping);
		damping *= damping * damping;
		
		acceleration *= attractor.strength * damping;
		
		result += acceleration;
	}
	
	return result;
}

float3 RandomSpherePoint(float3 random)
{
	return random * 2.0f - 1.0f.xxx;
}

Particle EmitParticle(float3 oldPosition, float index)
{
	uint2 texCoord = (uint2)(frac(oldPosition.xy + float2(index / maxParticlesNumber, 0.0f)) * perlinNoiseSize);
	float4 randomModifier = perlinNoise[texCoord];
	float4 random0_0 = frac(random0 + randomModifier);
	float4 random1_0 = frac(random1 + randomModifier);
	
	Particle newParticle = (Particle)0;
	newParticle.position = emitterOrigin + RandomSpherePoint(random0_0.xyz) * emitterRadius;
	newParticle.velocity = lerp(minParticleVelocity, maxParticleVelocity, random0_0.zwx);
	
	newParticle.rotation = lerp(minRotation, maxRotation, random1_0.x);
	newParticle.rotationSpeed = lerp(minRotationSpeed, maxRotationSpeed, random1_0.y);
	
	newParticle.size = lerp(minSize, maxSize, random1_0.zw);
	newParticle.startLife = lerp(minLifeSec, maxLifeSec, random1_0.y);
	
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
	
	[branch]
	if (particle.life <= 0.0f)
	{
		float emitProbability = averageParticleEmit * deltaTime;
		
		if (random0.z <= emitProbability)
			particle = EmitParticle(particle.position, (float)(particleIndex) + time);
	}
	else
	{
		uint2 texCoordXY = (uint2)(frac(particle.position.xy) * perlinNoiseSize);
		uint2 texCoordYZ = (uint2)(frac(particle.position.yz) * perlinNoiseSize.yy);
		uint2 texCoordZX = (uint2)(frac(particle.position.zx) * perlinNoiseSize.yx);
		
		float3 displacement = perlinNoise[texCoordXY].xyz;
		displacement += perlinNoise[texCoordYZ].xyz;
		displacement += perlinNoise[texCoordZX].xyz;
		
		displacement = (displacement * 2.0f / 3.0f - 1.0f.xxx) * particleTurbulence;
		
		float3 velocity = particle.velocity;
		float3 acceleration = SumAccelerations(particle.position);
		
		particle.position += velocity * deltaTime;
		particle.velocity += acceleration * deltaTime * deltaTime + displacement;
		particle.velocity *= particleDamping;
		
		particle.rotation += particle.rotationSpeed * deltaTime;
		particle.rotationSpeed *= particleDamping;
		
		particle.life -= deltaTime;
	}
	
	particleBuffer[particleIndex] = particle;
}
