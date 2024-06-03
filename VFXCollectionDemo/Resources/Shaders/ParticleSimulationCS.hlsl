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
	
	float3 previousPosition;
	float previousRotation;
	
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

Particle EmitParticle()
{
	Particle newParticle = (Particle)0;
	newParticle.position = emitterOrigin + RandomSpherePoint(random0.xyz) * emitterRadius;
	
	float3 velocity = lerp(minParticleVelocity, maxParticleVelocity, random0.zwx);
	
	newParticle.previousPosition = newParticle.position - velocity;
	
	newParticle.rotation = lerp(minRotation, maxRotation, random1.x);
	
	float rotationSpeed = lerp(minRotationSpeed, maxRotationSpeed, random1.y);
	
	newParticle.previousRotation = newParticle.rotation - rotationSpeed;
	
	newParticle.size = lerp(minSize, maxSize, random1.zw);
	newParticle.startLife = lerp(minLifeSec, maxLifeSec, random1.y);
	
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
			particle = EmitParticle();
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
		
		float3 velocity = particle.position - particle.previousPosition + displacement;
		float3 acceleration = SumAccelerations(particle.position);
		
		particle.previousPosition = particle.position;
		particle.position += (velocity * particleDamping + acceleration * deltaTime) * deltaTime;
		
		float angularSpeed = particle.rotation - particle.previousRotation;
		
		particle.previousRotation = particle.rotation;
		particle.rotation += angularSpeed * deltaTime;
		
		particle.life -= deltaTime;
	}
	
	particleBuffer[particleIndex] = particle;
}
