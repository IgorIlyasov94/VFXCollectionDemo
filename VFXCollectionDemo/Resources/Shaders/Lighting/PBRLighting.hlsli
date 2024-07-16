static const float PI = 3.14159265f;
static const float EPSILON = 0.59604645E-7f;

struct Material
{
	float3 albedo;
	float3 f0;
	float3 f90;
	float metalness;
	float roughness;
};

struct PointLight
{
	float3 position;
	float3 color;
};

struct Surface
{
	float3 position;
	float3 normal;
};

struct LightingDesc
{
	Surface surface;
	PointLight pointLight;
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

void CalculateLighting(LightingDesc lightingDesc, float3 viewDir, out float3 light)
{
	float3 position = lightingDesc.surface.position;
	float3 normal = lightingDesc.surface.normal;
	float3 lightVec = position - lightingDesc.pointLight.position;
	float3 lightDir = normalize(lightVec);
	float3 albedo = lerp(lightingDesc.material.albedo, 0.0f.xxx, lightingDesc.material.metalness);
	
	float attenuation = 1.0f / (dot(lightVec, lightVec) + 0.01f);
	
	float3 F;
	float3 F0 = lightingDesc.material.f0;
	float3 F90 = lightingDesc.material.f90;
	float roughness = lightingDesc.material.roughness;
	
	float3 specular = max(GGX_Specular(normal, viewDir, lightDir, roughness, F0, F90, F), 0.0f.xxx);
	float3 diffuse = max(GGX_Diffuse(normal, lightDir, F), 0.0f.xxx);
	
	light = (albedo * diffuse + specular) * lightingDesc.pointLight.color * attenuation;
}
