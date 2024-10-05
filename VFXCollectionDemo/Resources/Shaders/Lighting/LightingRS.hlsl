#include "PBRLighting.hlsli"

static const uint MAX_RECURSION_DEPTH = 3u;

static const uint INSTANCE_INCLUSION_MASK = ~0u;

static const uint HIT_GROUP_GEOMETRY_INDEX = 0u;
static const uint HIT_GROUP_SHADOW_INDEX = 1u;
static const uint HIT_GROUP_NUMBER = 2u;

static const uint MISS_GEOMETRY_INDEX = 0u;
static const uint MISS_SHADOW_INDEX = 1u;

static const uint HIT_KIND_CLOSEST = 0u;

static const uint LIGHT_SOURCE_NUMBER = 2u;

static const uint GEOMETRY_INDEX_STRIDE = 2u;
static const uint GEOMETRY_INDICES_PER_TRIANGLE = 3u;
static const uint GEOMETRY_TRIANGLE_STRIDE = GEOMETRY_INDEX_STRIDE * GEOMETRY_INDICES_PER_TRIANGLE;

static const float SHADOW_BIAS = 0.00001f;
static const float REFLECT_BIAS = 0.99999f;
static const float REFRACT_BIAS = 1.00001f;

static const float MAX_INNER_DISTANCE = 1.5f;

//https://refractiveindex.info/
static const float3 SAPPHIRE_IOR = float3(1.766f, 1.771f, 1.778f);
static const float3 SAPPHIRE_INV_IOR = float3(0.566f, 0.565f, 0.562f);
static const float3 SAPPHIRE_F0 = float3(0.1711f, 0.1730f, 0.1746f);
static const float3 SAPPHIRE_ABSORPTION_COEFF = float3(90.0f, 50.0f, 10.0f);
static const float3 SAPPHIRE_ALBEDO = float3(0.3f, 0.4f, 1.0f);

struct Vertex
{
	float3 position;
	uint normalXY;
	uint normalZW;
	uint tangentXY;
	uint tangentZW;
	uint texCoordXY;
};

struct UnpackedData
{
	float3 normal;
	float3 tangent;
	float2 texCoord;
};

struct Payload
{
    float3 color;
	float hitT;
	uint recursionDepth;
};

struct PayloadShadow
{
	float3 lightFactor;
};

cbuffer LightConstantBuffer : register(b0)
{
	PointLight lightSources[LIGHT_SOURCE_NUMBER];
};

cbuffer MutableConstants : register(b1)
{
	float4x4 invViewProjection;
	float3 cameraPosition;
	float padding;
};

RaytracingAccelerationStructure sceneStructure : register(t0);
StructuredBuffer<Vertex> sceneGeometry : register(t1);
ByteAddressBuffer sceneIndices : register(t2);
StructuredBuffer<Vertex> crystalGeometry : register(t3);
ByteAddressBuffer crystalIndices : register(t4);

Texture2D sceneAlbedoRoughness : register(t5);
Texture2D sceneNormalMetalness : register(t6);
Texture2D noiseMap : register(t7);

SamplerState samplerLinear : register(s0);
SamplerState samplerPoint : register(s1);

RWTexture2D<float4> resultTarget : register(u0);

void CalculateTangents(float3 normal, out float3 tangent, out float3 binormal)
{
	tangent = abs(normal.y) < 0.999f ? float3(0.0f, -1.0f, 0.0f) : float3(0.0f, 0.0f, 1.0f);
	tangent = normalize(cross(normal, tangent));
	
	binormal = cross(tangent, normal);
}

RayDesc BuildRay(uint2 rayIndex, uint2 raysNumber, float3 origin, float4x4 unprojectionMatrix)
{
	float4 screenPosition = float4(((float2)rayIndex + 0.5f.xx) / raysNumber, 0.0f, 1.0f);
	screenPosition.xy *= 2.0f.xx;
	screenPosition.xy -= 1.0f.xx;
	screenPosition.y = -screenPosition.y;
	
	float4 worldPosition = mul(unprojectionMatrix, screenPosition);
	worldPosition.xyz /= worldPosition.w;
	
	RayDesc rayDesc;
	rayDesc.Origin = origin;
	rayDesc.TMin = 0.0f;
	rayDesc.Direction = normalize(worldPosition.xyz - origin);
	rayDesc.TMax = 10000.0f;
	
	return rayDesc;
}

RayDesc BuildShadowRay(float3 origin, float3 lightPosition)
{
	float3 pointToLight = lightPosition - origin;
	float pointToLightLength = length(pointToLight);
	
	RayDesc rayDesc;
	rayDesc.Origin = origin;
	rayDesc.TMin = 0.0f;
	rayDesc.Direction = pointToLight / pointToLightLength;
	rayDesc.TMax = pointToLightLength;
	
	return rayDesc;
}

RayDesc BuildReflectRay(float3 origin, float3 incidentRayDirection, float3 normal, float3 random, float roughness)
{
	float3 rayDirection = reflect(incidentRayDirection, normal);
	
	float3 tangent = 0.0f.xxx;
	float3 binormal = 0.0f.xxx;
	CalculateTangents(rayDirection, tangent, binormal);
	
	float3 randomDirection = random;
	randomDirection.xy = (randomDirection.xy * 2.0f - 1.0f.xx) * 0.5f;
	randomDirection.z *= 0.5f;
	
	rayDirection = lerp(rayDirection, randomDirection, roughness * roughness);
	rayDirection = normalize(rayDirection);
	
	RayDesc rayDesc;
	rayDesc.Origin = origin;
	rayDesc.TMin = 0.0f;
	rayDesc.Direction = rayDirection;
	rayDesc.TMax = 10000.0f;
	
	return rayDesc;
}

RayDesc BuildRefractRay(float3 origin, float3 incidentRayDirection, float3 normal, float ior)
{
	float3 rayDirection = refract(incidentRayDirection, normal, ior);
	
	RayDesc rayDesc;
	rayDesc.Origin = origin;
	rayDesc.TMin = 0.0f;
	rayDesc.Direction = rayDirection;
	rayDesc.TMax = 10000.0f;
	
	return rayDesc;
}

uint3 GetIndices(uint byteIndex, ByteAddressBuffer rawIndexBuffer)
{
	uint3 indices;
	
	uint alignedIndex = byteIndex & ~3u;
    uint2 packedIndices = rawIndexBuffer.Load2(alignedIndex);
	
    if (alignedIndex == byteIndex)
    {
        indices.x = packedIndices.x & 0xffff;
        indices.y = (packedIndices.x >> 16) & 0xffff;
        indices.z = packedIndices.y & 0xffff;
    }
    else
    {
        indices.x = (packedIndices.x >> 16) & 0xffff;
        indices.y = packedIndices.y & 0xffff;
        indices.z = (packedIndices.y >> 16) & 0xffff;
    }

    return indices;
}

UnpackedData UnpackVertex(Vertex vertex)
{
	UnpackedData unpackedData = (UnpackedData)0;
	unpackedData.normal.x = f16tof32(vertex.normalXY & 0xffff);
	unpackedData.normal.y = f16tof32((vertex.normalXY >> 16) & 0xffff);
	unpackedData.normal.z = f16tof32(vertex.normalZW & 0xffff);
	unpackedData.tangent.x = f16tof32(vertex.tangentXY & 0xffff);
	unpackedData.tangent.y = f16tof32((vertex.tangentXY >> 16) & 0xffff);
	unpackedData.tangent.z = f16tof32(vertex.tangentZW & 0xffff);
	unpackedData.texCoord.x = f16tof32(vertex.texCoordXY & 0xffff);
	unpackedData.texCoord.y = f16tof32((vertex.texCoordXY >> 16) & 0xffff);
	
	return unpackedData;
}

Payload TraceLightRay(RayDesc rayDesc, uint recursionDepth, uint rayFlags)
{
	Payload payload = (Payload)0;
	
	if (recursionDepth >= MAX_RECURSION_DEPTH)
		return payload;
	
	payload.color = 0.0f.xxx;
	payload.hitT = 0.0f;
	payload.recursionDepth = recursionDepth + 1u;
	
	TraceRay(sceneStructure,
		RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
		INSTANCE_INCLUSION_MASK,
		HIT_GROUP_GEOMETRY_INDEX,
		HIT_GROUP_NUMBER,
		MISS_GEOMETRY_INDEX,
		rayDesc,
		payload);
	
	return payload;
}

float3 TraceShadowRay(RayDesc rayDesc, uint recursionDepth)
{
	if (recursionDepth > MAX_RECURSION_DEPTH)
		return false;
	
	PayloadShadow payload;
	payload.lightFactor = 0.0f.xxx;
	
	TraceRay(sceneStructure,
		RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH |
		RAY_FLAG_SKIP_CLOSEST_HIT_SHADER |
		RAY_FLAG_CULL_FRONT_FACING_TRIANGLES,
		INSTANCE_INCLUSION_MASK,
		HIT_GROUP_SHADOW_INDEX,
		HIT_GROUP_NUMBER,
		MISS_SHADOW_INDEX,
		rayDesc,
		payload);
	
	return payload.lightFactor;
}

float3 ProcessLightSource(PointLight lightSource, Surface surface, Material material, float3 rayDirection, uint recursionDepth, float3 random)
{
	float3 biasedSurfacePosition = surface.position;
	biasedSurfacePosition -= rayDirection * SHADOW_BIAS;
	
	RayDesc shadowRay = BuildShadowRay(biasedSurfacePosition, lightSource.position);
	float3 lightFactor = TraceShadowRay(shadowRay, recursionDepth);
	
	RayDesc reflectRay = BuildReflectRay(biasedSurfacePosition, rayDirection, surface.normal, random, material.roughness);
	Payload reflectLightPayload = TraceLightRay(reflectRay, recursionDepth, RAY_FLAG_CULL_BACK_FACING_TRIANGLES);
	
	PointLight reflectLightSource = (PointLight)0;
	reflectLightSource.position = surface.position + reflectRay.Direction * reflectLightPayload.hitT * REFLECT_BIAS;
	reflectLightSource.color = reflectLightPayload.color;
	
	float3 reflectLight = 0.0f.xxx;
	CalculatePointLight(reflectLightSource, surface, material, reflectRay.Direction, reflectLight);
	
	float3 light = 0.0f.xxx;
	CalculatePointLight(lightSource, surface, material, rayDirection, light);
	
	return light * lightFactor + reflectLight;
}

float3 BouguerLambertBeerLaw(float3 radiance, float thickness, float3 absorptionCoeff)
{
	return radiance * exp(-absorptionCoeff * thickness);
}

[shader("raygeneration")]
void RayGeneration()
{
	uint2 rayIndex = DispatchRaysIndex().xy;
	uint2 raysNumber = DispatchRaysDimensions().xy;
	RayDesc rayDesc = BuildRay(rayIndex, raysNumber, cameraPosition, invViewProjection);
	
	Payload resultPayload = TraceLightRay(rayDesc, 0u, RAY_FLAG_CULL_BACK_FACING_TRIANGLES);
	
	resultTarget[rayIndex] = float4(resultPayload.color, 1.0f);
}

[shader("closesthit")]
void ClosestHit(inout Payload payload, in BuiltInTriangleIntersectionAttributes attributes)
{
    float3 barycentrics;
	barycentrics.x = 1.0f - attributes.barycentrics.x - attributes.barycentrics.y;
	barycentrics.y = attributes.barycentrics.x;
	barycentrics.z = attributes.barycentrics.y;
	
    uint byteIndex = GEOMETRY_TRIANGLE_STRIDE * PrimitiveIndex();
	uint3 indices = GetIndices(byteIndex, sceneIndices);
	
	Vertex vertex0 = sceneGeometry[indices.x];
	Vertex vertex1 = sceneGeometry[indices.y];
	Vertex vertex2 = sceneGeometry[indices.z];
	
	UnpackedData unpackedData0 = UnpackVertex(vertex0);
	UnpackedData unpackedData1 = UnpackVertex(vertex1);
	UnpackedData unpackedData2 = UnpackVertex(vertex2);
	
	float3 macroNormal = barycentrics.x * unpackedData0.normal;
	macroNormal += barycentrics.y * unpackedData1.normal;
	macroNormal += barycentrics.z * unpackedData2.normal;
	macroNormal = normalize(macroNormal);
	
	float3 tangent = barycentrics.x * unpackedData0.tangent;
	tangent += barycentrics.y * unpackedData1.tangent;
	tangent += barycentrics.z * unpackedData2.tangent;
	tangent = normalize(macroNormal);
	
	float3 binormal = cross(tangent, macroNormal);
	
	float2 texCoord = barycentrics.x * unpackedData0.texCoord;
	texCoord += barycentrics.y * unpackedData1.texCoord;
	texCoord += barycentrics.z * unpackedData2.texCoord;
	
	float4 albedoRoughness = sceneAlbedoRoughness.SampleLevel(samplerLinear, texCoord, 0.0f);
	float4 normalMetalness = sceneNormalMetalness.SampleLevel(samplerLinear, texCoord, 0.0f);
	
	float3 albedo = albedoRoughness.xyz;
	float roughness = albedoRoughness.w;
	float3 normal = normalize(normalMetalness.xyz * 2.0f - 1.0f.xxx);
	float metalness = normalMetalness.w;
	
	normal = BumpMapping(normal, macroNormal, binormal, tangent);
	
	float3 rayDirection = WorldRayDirection();
	float hitT = RayTCurrent();
	float3 hitPosition = WorldRayOrigin() + hitT * rayDirection;
	
	Surface surface;
	surface.position = hitPosition;
	surface.normal = normal;
	
	Material material;
	material.albedo = albedo;
	material.f0 = 0.1f;
	material.f90 = 1.0f;
	material.metalness = metalness;
	material.roughness = roughness;
	
	float3 resultColor = 0.0f.xxx;
	
	float3 random = noiseMap.SampleLevel(samplerLinear, texCoord * 5.0f * payload.recursionDepth, 0.0f).xyz;
	
	[unroll]
	for (uint lightSourceIndex = 0u; lightSourceIndex < LIGHT_SOURCE_NUMBER; lightSourceIndex++)
		resultColor += ProcessLightSource(lightSources[lightSourceIndex], surface, material, rayDirection, payload.recursionDepth, random);
	
    payload.color += resultColor;
	payload.hitT = hitT;
}

[shader("closesthit")]
void CrystalClosestHit(inout Payload payload, in BuiltInTriangleIntersectionAttributes attributes)
{
	float3 barycentrics;
	barycentrics.x = 1.0f - attributes.barycentrics.x - attributes.barycentrics.y;
	barycentrics.y = attributes.barycentrics.x;
	barycentrics.z = attributes.barycentrics.y;
	
    uint byteIndex = GEOMETRY_TRIANGLE_STRIDE * PrimitiveIndex();
	uint3 indices = GetIndices(byteIndex, crystalIndices);
	
	Vertex vertex0 = crystalGeometry[indices.x];
	Vertex vertex1 = crystalGeometry[indices.y];
	Vertex vertex2 = crystalGeometry[indices.z];
	
	UnpackedData unpackedData0 = UnpackVertex(vertex0);
	UnpackedData unpackedData1 = UnpackVertex(vertex1);
	UnpackedData unpackedData2 = UnpackVertex(vertex2);
	
	float3 normal = barycentrics.x * unpackedData0.normal;
	normal += barycentrics.y * unpackedData1.normal;
	normal += barycentrics.z * unpackedData2.normal;
	normal = normalize(normal);
	
	float3 rayDirection = WorldRayDirection();
	float hitT = RayTCurrent();
	float3 hitPosition = WorldRayOrigin() + hitT * rayDirection * REFRACT_BIAS;
	
	bool isIncident = dot(normal, rayDirection) < 0.0f;
	bool isInner = hitT < MAX_INNER_DISTANCE;
	
	normal = isIncident ? normal : -normal;
	float3 ior = isInner ? 1.0f : isIncident ? SAPPHIRE_INV_IOR : SAPPHIRE_IOR;
	
	Surface surface;
	surface.position = hitPosition;
	surface.normal = normal;
	
	Material material;
	material.albedo = SAPPHIRE_ALBEDO;
	material.f0 = SAPPHIRE_F0;
	material.f90 = 1.0f;
	material.metalness = 0.0f;
	material.roughness = 0.001f;
	
	float3 lighting = 0.0f.xxx;
	[unroll]
	for (uint lightSourceIndex = 0u; lightSourceIndex < LIGHT_SOURCE_NUMBER; lightSourceIndex++)
		lighting += ProcessLightSource(lightSources[lightSourceIndex], surface, material, rayDirection, payload.recursionDepth, 0.5f.xxx);
	
	RayDesc rayRedDesc = BuildRefractRay(hitPosition, rayDirection, normal, ior.x);
	RayDesc rayBlueDesc = BuildRefractRay(hitPosition, rayDirection, normal, ior.y);
	RayDesc rayGreenDesc = BuildRefractRay(hitPosition, rayDirection, normal, ior.z);
	
	Payload payloadRed = TraceLightRay(rayRedDesc, payload.recursionDepth, RAY_FLAG_NONE);
	Payload payloadGreen = TraceLightRay(rayBlueDesc, payload.recursionDepth, RAY_FLAG_NONE);
	Payload payloadBlue = TraceLightRay(rayGreenDesc, payload.recursionDepth, RAY_FLAG_NONE);
	
	payload.color = float3(payloadRed.color.x, payloadGreen.color.y, payloadBlue.color.z);
	payload.color *= isInner ? BouguerLambertBeerLaw(payload.color, hitT, SAPPHIRE_ABSORPTION_COEFF) : SAPPHIRE_ALBEDO;
	payload.color += lighting;
	payload.recursionDepth = payloadRed.recursionDepth;
	payload.hitT = hitT;
}

[shader("anyhit")]
void CrystalAnyHit_Shadow(inout PayloadShadow payload, in BuiltInTriangleIntersectionAttributes attributes)
{
	float3 barycentrics;
	barycentrics.x = 1.0f - attributes.barycentrics.x - attributes.barycentrics.y;
	barycentrics.y = attributes.barycentrics.x;
	barycentrics.z = attributes.barycentrics.y;
	
    uint byteIndex = GEOMETRY_TRIANGLE_STRIDE * PrimitiveIndex();
	uint3 indices = GetIndices(byteIndex, crystalIndices);
	
	Vertex vertex0 = crystalGeometry[indices.x];
	Vertex vertex1 = crystalGeometry[indices.y];
	Vertex vertex2 = crystalGeometry[indices.z];
	
	UnpackedData unpackedData0 = UnpackVertex(vertex0);
	UnpackedData unpackedData1 = UnpackVertex(vertex1);
	UnpackedData unpackedData2 = UnpackVertex(vertex2);
	
	float3 normal = barycentrics.x * unpackedData0.normal;
	normal += barycentrics.y * unpackedData1.normal;
	normal += barycentrics.z * unpackedData2.normal;
	normal = normalize(normal);
	
	float3 rayDirection = WorldRayDirection();
	float dotNR = dot(normal, rayDirection);
	dotNR *= dotNR;
	
	float hitT = RayTCurrent();
	
	payload.lightFactor = SAPPHIRE_ALBEDO * dotNR;
}

[shader("miss")]
void Miss(inout Payload payload)
{
    payload.color = 0.0f.xxx;
}

[shader("miss")]
void Miss_Shadow(inout PayloadShadow payload)
{
    payload.lightFactor = 1.0f.xxx;
}
