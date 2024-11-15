static const uint AREA_LIGHT_SAMPLES_NUMBER = 8;

static const float PI = 3.14159265f;
static const float EPSILON = 0.59604645E-7f;

static const float MIE_G = -0.99f;
static const float MIE_G2 = 0.9801f;

static const float HG_G = -0.99f;
static const float HG_FORWARD_G = HG_G;
static const float HG_BACKWARD_G = -HG_G / 2.0f;
static const float HG_LERP_FACTOR = 1.0f - HG_BACKWARD_G * HG_BACKWARD_G;

static const float SCATTERING_CONST = 0.001f;

static const float AIR_PARTICLE_SIZE = 0.1578E-9;
static const float FOG_WATER_PARTICLE_SIZE = 10E-6;
static const float CLOUD_LOWER_WATER_PARTICLE_SIZE = 5E-6;
static const float CLOUD_UPPER_WATER_PARTICLE_SIZE = 30E-6;

static const float3 AIR_IOR = float3(1.00027657f, 1.00027784f, 1.00028004f);
static const float3 WATER_IOR = float3(1.3318f, 1.3330f, 1.3364f);

static const float3 AIR_ABSORPTION_COEFF = float3(0.6f, 0.6f, 0.6f);
static const float3 AIR_WET_ABSORPTION_COEFF = float3(1.2f, 1.2f, 1.2f);
static const float3 WATER_ABSORPTION_COEFF = float3(2.8723f, 0.44782f, 0.26335f) * 3E-3;

static const float3 INV_WAVELENGTH_4 = 1.0f.xxx / pow(float3(630E-3, 550E-3, 464E-3), 4.0f);

static const float AREA_LIGHT_SAMPLE_WEIGHT = 1.0f / AREA_LIGHT_SAMPLES_NUMBER;

static const float3 LUMINANCE_VECTOR = float3(0.2126f, 0.7152f, 0.0722f);

static const float3 AREA_LIGHT_SAMPLES[AREA_LIGHT_SAMPLES_NUMBER] =
{
	float3(0.154508f, 0.475528f, 0.000000f),
	float3(-0.286031f, 0.207813f, 0.353553f),
	float3(-0.286031f, -0.207814f, -0.353553f),
	float3(0.059128f, -0.181977f, 0.461940f),
	
	float3(-0.309017f, -0.951056f, 0.000000f),
	float3(0.572062f, -0.415627f, 0.707107f),
	float3(0.572061f, 0.415627f, -0.707107f),
	float3(-0.118256f, 0.363954f, 0.923880f)
};

struct DirectionalLight
{
	float3 direction;
	float padding0;
	float3 color;
	float padding1;
};

struct PointLight
{
	float3 position;
	float range;
	float3 color;
	float padding1;
};

struct AreaLight
{
	float3 position;
	float radius;
	float3 color;
	float range;
};

struct SpotLight
{
	float3 position;
	float range;
	float3 color;
	float cosPhi2;
	float3 direction;
	float cosTheta2;
};

struct AmbientLight
{
	float3 color;
	float padding;
};

struct Material
{
	float3 albedo;
	float3 f0;
	float3 f90;
	float metalness;
	float roughness;
};

struct ShadowData
{
	float4x4 lightViewProjection;
	Texture2D shadowMap;
	SamplerComparisonState shadowSampler;
};

struct ShadowCubeData
{
	float zNear;
	float zFar;
	TextureCube shadowMap;
	SamplerComparisonState shadowSampler;
};

struct LightData
{
	float3 direction;
	float3 color;
	float attenuationCoeff;
};

struct Surface
{
	float3 position;
	float3 normal;
};

struct LightingDesc
{
	Surface surface;
	LightData lightData;
	Material material;
};

float3 BumpMapping(float3 texSpaceNormal, float3 normal, float3 binormal, float3 tangent)
{
	return normalize(tangent * texSpaceNormal.x + binormal * texSpaceNormal.y + normal * texSpaceNormal.z);
}

float3 FresnelSchlick(float3 R0, float3 R90, float dotHV)
{
    float t = pow(1.0f - saturate(dotHV), 5.0f);
    return lerp(R0, R90, t);
}

float GGX_Distribution(float dotNH, float sqrRoughness)
{
	float sqrDotNH = saturate(dotNH * dotNH);
    float d = sqrDotNH * sqrRoughness + (1.0h - sqrDotNH);
    float d2 = d * d;

    return saturate(sqrRoughness / max(PI * d2, EPSILON));
}

float GGX_Geometry(float dotXY, float sqrRoughness)
{
	float sqrDotXY = saturate(dotXY * dotXY);
    return saturate(2.0h / max(1.0f + sqrt(lerp(sqrRoughness, 1.0h, sqrDotXY)) / dotXY, EPSILON));
}

float3 GGX_Specular(float3 N, float3 V, float3 L, float roughness, float3 F0, float3 F90, out float3 F)
{
	float3 invV = -V;
	float3 invL = -L;
    float3 H = normalize(invL + N);
	
    float shiftAmount = dot(N, invV);
    N = shiftAmount < 0.0f ? N + invV * (-shiftAmount + EPSILON) : N;
	
    float dotNL = max(dot(N, invL), EPSILON);
    float dotNV = max(dot(N, invV), EPSILON);
    float dotHV = dot(H, invV);
    float dotNH = dot(N, H);

    float sqrRoughness = roughness * roughness;

    F = FresnelSchlick(F0, F90, dotHV);
    float G = GGX_Geometry(dotNL, sqrRoughness) * GGX_Geometry(dotNV, sqrRoughness);
    float D = GGX_Distribution(dotNH, sqrRoughness);

    return F * G * D / (4.0f * dotNV * dotNL);
}

float3 GGX_Diffuse(float3 N, float3 L, float3 F)
{
	float3 invL = -L;
	
    return saturate(dot(N, invL)) * saturate(1.0f.xxx - F);
}

float3 GGX_AmbientDiffuse(float3 N, float3 V, float3 F0, float3 F90)
{
	float3 invV = -V;
    
    float shiftAmount = dot(N, invV);
    N = shiftAmount < 0.0f ? N + invV * (-shiftAmount + EPSILON) : N;
	float dotNV = max(dot(N, invV), EPSILON);
	
	float3 F = FresnelSchlick(F0, F90, dotNV);
    
    return saturate(N.z * 0.5f + 0.5f) * saturate(1.0f.xxx - F);
}

void CalculatePBRLighting(LightingDesc lightingDesc, float3 viewDir, out float3 light)
{
	float3 position = lightingDesc.surface.position;
	float3 normal = lightingDesc.surface.normal;
	float3 lightDir = lightingDesc.lightData.direction;
	float3 albedo = lightingDesc.material.albedo;
	
	float attenuation = lightingDesc.lightData.attenuationCoeff;
	
	float3 F;
	float3 F0 = lerp(lightingDesc.material.f0, albedo, lightingDesc.material.metalness);
	float3 F90 = lightingDesc.material.f90;
	float roughness = lightingDesc.material.roughness;
	
	float3 specular = max(GGX_Specular(normal, viewDir, lightDir, roughness, F0, F90, F), 0.0f.xxx);
	float3 diffuse = max(GGX_Diffuse(normal, lightDir, F), 0.0f.xxx);
	
	float intensity = dot(lightingDesc.lightData.color, LUMINANCE_VECTOR);
	
	float3 diffuseColor = lightingDesc.lightData.color;
	float3 specularColor = lerp(intensity.xxx, lightingDesc.lightData.color, lightingDesc.material.metalness);
	
	light = (albedo * diffuse * diffuseColor + specular * specularColor) * attenuation;
}

void CalculatePBRAmbient(LightingDesc lightingDesc, float3 viewDir, out float3 light)
{
	float3 normal = lightingDesc.surface.normal;
	float3 albedo = lerp(lightingDesc.material.albedo, 0.0f.xxx, lightingDesc.material.metalness);
	
	float3 F0 = lightingDesc.material.f0;
	float3 F90 = lightingDesc.material.f90;
	
	float3 diffuse = max(GGX_AmbientDiffuse(normal, viewDir, F0, F90), 0.0f.xxx);
	
	light = albedo * diffuse * lightingDesc.lightData.color;
}

float LightVectorToDepth(float3 lightVector, float zNear, float zFar)
{
	float3 absLightVector = abs(lightVector);
	float dist = max(absLightVector.x, max(absLightVector.y, absLightVector.z));
	
	float coeff0 = zFar / (zNear - zFar);
	float coeff1 = zNear * zFar / (zNear - zFar);
	return -coeff0 + coeff1 / dist - 0.0001f;
}

float CalculateShadowCube(ShadowCubeData shadowData, float3 worldPosition, float3 lightPosition)
{
	uint status;
	
	float3 lightVector = worldPosition - lightPosition;
	float3 direction = normalize(lightVector);
	direction = direction.xzy;
	
	float sceneDepth = LightVectorToDepth(lightVector, shadowData.zNear, shadowData.zFar);
	float4 occlusions = shadowData.shadowMap.GatherCmpRed(shadowData.shadowSampler, direction, sceneDepth, status);
	
	return dot(occlusions, 0.25f.xxxx);
}

float RayleighScatteringPhase(float cosAngle2)
{
	return 1.0f + cosAngle2;
}

float MieScatteringPhase(float cosAngle, float cosAngle2)
{
	float phase = max(1.0f - 2.0f * MIE_G * cosAngle + MIE_G2, 1.0E-4);
	phase = 1.5f * ((1.0f - MIE_G2) / (2.0f + MIE_G2)) * (1.0f + cosAngle2) / phase;
	
	return phase;
}

float HenyeyGreensteinPhase(float cosAngle, float g)
{
	float phase = 1.0f - g * g;
	phase *= pow(1.0f - 2.0f * g * cosAngle + g * g, -1.5f);
	
	return phase;
}

float DoubleHenyeyGreensteinPhase(float cosAngle)
{
	float forwardPhase = HenyeyGreensteinPhase(cosAngle, HG_FORWARD_G);
	float backwardPhase = HenyeyGreensteinPhase(cosAngle, HG_BACKWARD_G);
	
	return lerp(backwardPhase, forwardPhase, HG_LERP_FACTOR);
}

float4 BouguerLambertBeerLaw(float4 radiance, float thickness, float4 absorptionCoeff)
{
	return radiance * exp(-absorptionCoeff * thickness);
}

float3 BouguerLambertBeerLaw(float3 radiance, float thickness, float3 absorptionCoeff)
{
	return radiance * exp(-absorptionCoeff * thickness);
}

float3 BouguerLambertBeerLaw(float thickness, float3 absorptionCoeff)
{
	return exp(-absorptionCoeff * thickness);
}

float Attenuation(float x, float range)
{
	return exp(-x * 5.5f / range);
}

void CalculateDirectionalLight(DirectionalLight directionalLight, Surface surface, Material material, float3 viewDir, out float3 light)
{
	LightData lightData;
	lightData.direction = directionalLight.direction;
	lightData.color = directionalLight.color;
	lightData.attenuationCoeff = 1.0f;
	
	LightingDesc lightingDesc;
	lightingDesc.surface = surface;
	lightingDesc.lightData = lightData;
	lightingDesc.material = material;
	
	CalculatePBRLighting(lightingDesc, viewDir, light);
}

void CalculatePointLight(PointLight pointLight, Surface surface, Material material, float3 viewDir, out float3 light)
{
	float3 lightToSurface = surface.position - pointLight.position;
	float lightDistance = length(lightToSurface);
	
	LightData lightData;
	lightData.direction = lightDistance > 0.0f ? lightToSurface / lightDistance : float3(0.0f, 1.0f, 0.0f);
	lightData.color = pointLight.color;
	lightData.attenuationCoeff = Attenuation(lightDistance, pointLight.range);
	
	LightingDesc lightingDesc;
	lightingDesc.surface = surface;
	lightingDesc.lightData = lightData;
	lightingDesc.material = material;
	
	CalculatePBRLighting(lightingDesc, viewDir, light);
}

void CalculateAreaLight(AreaLight areaLight, Surface surface, Material material, float3 viewDir, out float3 light)
{
	light = 0.0f.xxx;
	
	[unroll]
	for (uint pointIndex = 0; pointIndex < AREA_LIGHT_SAMPLES_NUMBER; pointIndex++)
	{
		float3 lightPoint = areaLight.position + AREA_LIGHT_SAMPLES[pointIndex] * areaLight.radius;
		
		float3 lightToSurface = surface.position - areaLight.position;
		float lightDistance = length(lightToSurface);
		
		LightData lightData;
		lightData.direction = lightDistance > 0.0f ? lightToSurface / lightDistance : float3(0.0f, 1.0f, 0.0f);
		lightData.color = areaLight.color * AREA_LIGHT_SAMPLE_WEIGHT;
		lightData.attenuationCoeff = Attenuation(lightDistance, areaLight.range);
		
		LightingDesc lightingDesc;
		lightingDesc.surface = surface;
		lightingDesc.lightData = lightData;
		lightingDesc.material = material;
		
		float3 lightRate = 0.0f.xxx;
		CalculatePBRLighting(lightingDesc, viewDir, lightRate);
		
		light += lightRate;
	}
}

void CalculateSpotLight(SpotLight spotLight, Surface surface, Material material, float3 viewDir, out float3 light)
{
	float3 lightToSurface = surface.position - spotLight.position;
	float lightDistance = length(lightToSurface);
	
	LightData lightData;
	lightData.direction = lightDistance > 0.0f ? lightToSurface / lightDistance : float3(0.0f, 1.0f, 0.0f);
	lightData.color = spotLight.color;
	
	float cosAlpha = dot(lightData.direction, spotLight.direction);
	float cosPhi2 = spotLight.cosPhi2;
	float cosTheta2 = spotLight.cosTheta2;
	float spotAttenuation = saturate((cosAlpha - cosPhi2) / (cosTheta2 - cosPhi2));
	spotAttenuation *= spotAttenuation;
	
	lightData.attenuationCoeff = Attenuation(lightDistance, spotLight.range);
	lightData.attenuationCoeff *= spotAttenuation;
	
	LightingDesc lightingDesc;
	lightingDesc.surface = surface;
	lightingDesc.lightData = lightData;
	lightingDesc.material = material;
	
	CalculatePBRLighting(lightingDesc, viewDir, light);
}

void CalculateAmbientLight(AmbientLight ambientLight, Surface surface, Material material, float3 viewDir, out float3 light)
{
	LightData lightData = (LightData)0;
	lightData.color = ambientLight.color;
	
	LightingDesc lightingDesc;
	lightingDesc.surface = surface;
	lightingDesc.lightData = lightData;
	lightingDesc.material = material;
	
	CalculatePBRAmbient(lightingDesc, viewDir, light);
}

void CalculateAreaLight(ShadowCubeData shadowData, AreaLight areaLight, Surface surface, Material material, float3 viewDir, out float3 light)
{
	light = 0.0f.xxx;
	
	[unroll]
	for (uint pointIndex = 0; pointIndex < AREA_LIGHT_SAMPLES_NUMBER; pointIndex++)
	{
		float3 offset = AREA_LIGHT_SAMPLES[pointIndex] * areaLight.radius;
		float3 lightPoint = areaLight.position + offset;
		
		float3 lightToSurface = surface.position - lightPoint;
		float lightDistance = length(lightToSurface);
		
		LightData lightData;
		lightData.direction = lightDistance > 0.0f ? lightToSurface / lightDistance : float3(0.0f, 1.0f, 0.0f);
		lightData.color = areaLight.color * AREA_LIGHT_SAMPLE_WEIGHT;
		lightData.attenuationCoeff = Attenuation(lightDistance, areaLight.range);
		
		LightingDesc lightingDesc;
		lightingDesc.surface = surface;
		lightingDesc.lightData = lightData;
		lightingDesc.material = material;
		
		float3 lightRate = 0.0f.xxx;
		CalculatePBRLighting(lightingDesc, viewDir, lightRate);
		
		float3 offsettedWorldPosition = surface.position - offset;
		float occlusion = CalculateShadowCube(shadowData, offsettedWorldPosition, areaLight.position);
		
		light += lightRate * occlusion;
	}
}
