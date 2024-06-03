static const float PI = 3.14159265f;
static const float EPSILON = 0.59604645E-7f;

cbuffer MutableConstants : register(b0)
{
	float4x4 world;
	float4x4 viewProj;
	float4 cameraDirection;
	float time;
	float3 padding;
};

struct Input
{
	float4 position : SV_Position;
	float3 normal : NORMAL;
	float3 binormal : BINORMAL;
	float3 tangent : TANGENT;
	float2 texCoord : TEXCOORD0;
	float3 worldPosition : TEXCOORD1;
};

struct Output
{
	float4 color : SV_TARGET0;
};

Texture2D albedoRoughness : register(t0);
Texture2D normalMetalness : register(t1);

SamplerState samplerLinear : register(s0);

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

[earlydepthstencil]
Output main(Input input)
{
	Output output = (Output)0;
	
	float4 texData = albedoRoughness.Sample(samplerLinear, input.texCoord);
	float3 albedo = texData.xyz;
	float roughness = texData.w * texData.w;
	
	texData = normalMetalness.Sample(samplerLinear, input.texCoord);
	float3 normal = texData.xyz * 2.0f - 1.0f.xxx;
	normal = BumpMapping(normal, input.normal, input.binormal, input.tangent);
	float metalness = texData.w;
	
	Surface surface;
	surface.position = input.worldPosition;
	surface.normal = normal;
	
	Material material;
	material.albedo = albedo;
	material.f0 = float3(0.0609f, 0.0617f, 0.0634f);
	material.f90 = 1.00f.xxx;
	material.metalness = metalness;
	material.roughness = roughness;
	
	float3 lightSum = 0.0f.xxx;
	
	for (int i = 0; i < 8; i++)
	{
		PointLight pointLight;
		pointLight.position = float3(sin(time * 3.0f + i * 1.28f) * 0.3f * i, 0.0f, 0.686f * i + 0.2f);
		pointLight.color = lerp(float3(0.93f, 0.074f, 0.074f), float3(0.1f, 0.044f, 0.3f), i / 7.0f);
		pointLight.color *= saturate(cos(time * 3.0f + i * 1.28f) * 0.25f * i + 0.75f);
		
		LightingDesc lightingDesc;
		lightingDesc.surface = surface;
		lightingDesc.pointLight = pointLight;
		lightingDesc.material = material;
		
		float3 light;
		CalculateLighting(lightingDesc, cameraDirection.xyz, light);
		
		lightSum += light;
	}
	
	float3 ambient = lerp(float3(0.4f, 0.35f, 0.1f), float3(0.5f, 0.6f, 0.65f), saturate(normal.z * 0.5f + 0.5f));
	
	output.color = float4(lightSum + albedo * ambient * 1.0f, 1.0f);
	
	return output;
}
