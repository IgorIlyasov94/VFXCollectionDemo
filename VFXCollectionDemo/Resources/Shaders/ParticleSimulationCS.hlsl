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

float3 RandomSpherePoint(float3 random)
{
	return random * 2.0f - 1.0f.xxx;
}

Particle EmitParticle(float4 random0_0, float4 random1_0)
{
	Particle newParticle = (Particle)0;
	newParticle.position = emitterOrigin + RandomSpherePoint(random0_0.xyz) * emitterRadius;
	newParticle.velocity = lerp(minParticleVelocity, maxParticleVelocity, random1_0.wxy);
	
	newParticle.rotation = lerp(minRotation, maxRotation, random1_0.x);
	newParticle.rotationSpeed = lerp(minRotationSpeed, maxRotationSpeed, random1_0.y);
	
	newParticle.size = lerp(minSize, maxSize, random1_0.z);
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
	
	[branch]
	if (particle.life <= 0.0f)
	{
		float emitProbability = averageParticleEmit * deltaTime;
		
		if (random0_0.z <= emitProbability)
			particle = EmitParticle(random0_0, random1_0);
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
