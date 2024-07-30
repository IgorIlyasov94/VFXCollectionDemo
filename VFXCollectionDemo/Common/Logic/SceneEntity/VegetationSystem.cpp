#include "VegetationSystem.h"
#include "LightingSystem.h"
#include "../../../Graphics/Assets/MaterialBuilder.h"
#include "../../../Graphics/Assets/Loaders/OBJLoader.h"
#include "../../../Graphics/Assets/Loaders/DDSLoader.h"
#include "../../../Graphics/Assets/GeometryUtilities.h"
#include "../../Utilities.h"

using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace Common;
using namespace Graphics;
using namespace Graphics::Resources;
using namespace Graphics::Assets;
using namespace Graphics::Assets::Loaders;

Common::Logic::SceneEntity::VegatationSystem::VegatationSystem(ID3D12GraphicsCommandList* commandList,
	Graphics::DirectX12Renderer* renderer, const VegetationSystemDesc& desc, const Camera* camera)
	: materialDepthPass(nullptr), materialDepthCubePass(nullptr)
{
	_camera = camera;
	lightConstantBufferId = desc.lightConstantBufferId;
	lightMatricesConstantBufferId = desc.lightMatricesConstantBufferId;

	std::random_device randomDevice;
	randomEngine = std::mt19937(randomDevice());

	auto device = renderer->GetDevice();
	auto resourceManager = renderer->GetResourceManager();

	GenerateMesh(device, commandList, resourceManager);
	CreateConstantBuffers(device, commandList, resourceManager, desc);
	CreateBuffers(device, commandList, resourceManager, desc);
	LoadShaders(device, resourceManager, desc);
	LoadTextures(device, commandList, resourceManager, desc);
	CreateMaterials(device, resourceManager, desc);
}

Common::Logic::SceneEntity::VegatationSystem::~VegatationSystem()
{

}

void Common::Logic::SceneEntity::VegatationSystem::Update(float time, float deltaTime)
{
	mutableConstantsBuffer->viewProjection = _camera->GetViewProjection();
	mutableConstantsBuffer->cameraPosition = _camera->GetPosition();
	mutableConstantsBuffer->time = time;
	mutableConstantsBuffer->windStrength = *windStrength;
}

void Common::Logic::SceneEntity::VegatationSystem::OnCompute(ID3D12GraphicsCommandList* commandList)
{

}

void Common::Logic::SceneEntity::VegatationSystem::DrawDepthPrepass(ID3D12GraphicsCommandList* commandList)
{
	materialDepthPrepass->Set(commandList);
	_mesh->Draw(commandList, instancesNumber);
}

void Common::Logic::SceneEntity::VegatationSystem::DrawShadows(ID3D12GraphicsCommandList* commandList,
	uint32_t lightMatrixStartIndex)
{
	auto lightMatrixIndex = lightMatrixStartIndex;

	materialDepthPass->Set(commandList);
	materialDepthPass->SetRootConstant(commandList, 0u, &lightMatrixIndex);
	_mesh->Draw(commandList, instancesNumber);
}

void Common::Logic::SceneEntity::VegatationSystem::DrawShadowsCube(ID3D12GraphicsCommandList* commandList,
	uint32_t lightMatrixStartIndex)
{
	auto lightMatrixIndex = lightMatrixStartIndex;

	materialDepthCubePass->Set(commandList);
	materialDepthCubePass->SetRootConstant(commandList, 0u, &lightMatrixIndex);
	_mesh->Draw(commandList, instancesNumber);
}

void Common::Logic::SceneEntity::VegatationSystem::Draw(ID3D12GraphicsCommandList* commandList)
{
	_material->Set(commandList);
	_mesh->Draw(commandList, instancesNumber);
}

void Common::Logic::SceneEntity::VegatationSystem::Release(Graphics::Resources::ResourceManager* resourceManager)
{
	resourceManager->DeleteResource<ConstantBuffer>(mutableConstantsId);
	resourceManager->DeleteResource<Buffer>(vegetationBufferId);
	resourceManager->DeleteResource<Texture>(albedoMapId);
	resourceManager->DeleteResource<Texture>(normalMapId);

	resourceManager->DeleteResource<Shader>(vegetationVSId);
	resourceManager->DeleteResource<Shader>(vegetationPSId);
	resourceManager->DeleteResource<Shader>(vegetationDepthPrepassVSId);
	resourceManager->DeleteResource<Shader>(vegetationDepthPassVSId);
	resourceManager->DeleteResource<Shader>(vegetationDepthPassPSId);
	resourceManager->DeleteResource<Shader>(vegetationDepthCubePassVSId);
	resourceManager->DeleteResource<Shader>(vegetationDepthCubePassGSId);
	resourceManager->DeleteResource<Shader>(vegetationDepthCubePassPSId);

	_mesh->Release(resourceManager);
	delete _mesh;

	delete _material;
	delete materialDepthPrepass;

	if (materialDepthPass != nullptr)
		delete materialDepthPass;

	if (materialDepthCubePass != nullptr)
		delete materialDepthCubePass;
}

void Common::Logic::SceneEntity::VegatationSystem::GenerateMesh(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, Graphics::Resources::ResourceManager* resourceManager)
{
	BufferDesc vbDesc{};
	vbDesc.numElements = 8u;
	vbDesc.dataStride = sizeof(GrassVertex);
	vbDesc.data.resize(static_cast<size_t>(vbDesc.dataStride) * vbDesc.numElements);

	auto halfZero = XMConvertFloatToHalf(0.0f);
	auto halfOne = XMConvertFloatToHalf(1.0f);

	auto nTop0 = GeometryUtilities::Vector3ToHalf4(float3(0.0f, -0.7071068f, 0.7071068f));
	auto nTop1 = GeometryUtilities::Vector3ToHalf4(float3(0.0f, -0.7071068f, 0.7071068f));
	auto nTop2 = GeometryUtilities::Vector3ToHalf4(float3(0.0f, 0.7071068f, 0.7071068f));
	auto nTop3 = GeometryUtilities::Vector3ToHalf4(float3(0.0f, 0.7071068f, 0.7071068f));
	auto nBottom0 = GeometryUtilities::Vector3ToHalf4(float3(0.0f, -0.7071068f, 0.7071068f));
	auto nBottom1 = GeometryUtilities::Vector3ToHalf4(float3(0.0f, -0.7071068f, 0.7071068f));
	auto nBottom2 = GeometryUtilities::Vector3ToHalf4(float3(0.0f, 0.7071068f, 0.7071068f));
	auto nBottom3 = GeometryUtilities::Vector3ToHalf4(float3(0.0f, 0.7071068f, 0.7071068f));

	auto tTop0 = GeometryUtilities::CalculateTangentHalf(float3(0.0f, -0.7071068f, 0.7071068f));
	auto tTop1 = GeometryUtilities::CalculateTangentHalf(float3(0.0f, -0.7071068f, 0.7071068f));
	auto tTop2 = GeometryUtilities::CalculateTangentHalf(float3(0.0f, 0.7071068f, 0.7071068f));
	auto tTop3 = GeometryUtilities::CalculateTangentHalf(float3(0.0f, 0.7071068f, 0.7071068f));
	auto tBottom0 = GeometryUtilities::CalculateTangentHalf(float3(0.0f, -0.7071068f, 0.7071068f));
	auto tBottom1 = GeometryUtilities::CalculateTangentHalf(float3(0.0f, -0.7071068f, 0.7071068f));
	auto tBottom2 = GeometryUtilities::CalculateTangentHalf(float3(0.0f, 0.7071068f, 0.7071068f));
	auto tBottom3 = GeometryUtilities::CalculateTangentHalf(float3(0.0f, 0.7071068f, 0.7071068f));

	auto vertices = reinterpret_cast<GrassVertex*>(vbDesc.data.data());
	vertices[0u] = SetVertex(float3(-0.5f, 0.0f, 1.0f), nTop0, tTop0, XMHALF2(halfZero, halfZero));
	vertices[1u] = SetVertex(float3(0.5f, 0.0f, 1.0f), nTop1, tTop1, XMHALF2(halfOne, halfZero));
	vertices[2u] = SetVertex(float3(0.5f, 0.0f, 0.0f), nBottom0, tBottom0, XMHALF2(halfOne, halfOne));
	vertices[3u] = SetVertex(float3(-0.5f, 0.0f, 0.0f), nBottom1, tBottom1, XMHALF2(halfZero, halfOne));
	vertices[4u] = SetVertex(float3(0.5f, 0.0f, 1.0f), nTop2, tTop2, XMHALF2(halfOne, halfZero));
	vertices[5u] = SetVertex(float3(-0.5f, 0.0f, 1.0f), nTop3, tTop3, XMHALF2(halfZero, halfZero));
	vertices[6u] = SetVertex(float3(-0.5f, 0.0f, 0.0f), nBottom2, tBottom2, XMHALF2(halfZero, halfOne));
	vertices[7u] = SetVertex(float3(0.5f, 0.0f, 0.0f), nBottom3, tBottom3, XMHALF2(halfOne, halfOne));

	auto vertexBufferId = resourceManager->CreateBufferResource(device, commandList,
		BufferResourceType::VERTEX_BUFFER, vbDesc);

	BufferDesc ibDesc{};
	ibDesc.numElements = 12u;
	ibDesc.dataStride = sizeof(uint16_t);
	ibDesc.data.resize(static_cast<size_t>(ibDesc.dataStride) * ibDesc.numElements);

	auto indices = reinterpret_cast<uint16_t*>(ibDesc.data.data());
	indices[0u] = 0u;
	indices[1u] = 1u;
	indices[2u] = 2u;
	indices[3u] = 2u;
	indices[4u] = 3u;
	indices[5u] = 0u;
	indices[6u] = 4u;
	indices[7u] = 5u;
	indices[8u] = 6u;
	indices[9u] = 6u;
	indices[10u] = 7u;
	indices[11u] = 4u;

	auto indexBufferId = resourceManager->CreateBufferResource(device, commandList,
		BufferResourceType::INDEX_BUFFER, ibDesc);

	MeshDesc meshDesc{};
	meshDesc.verticesNumber = vbDesc.numElements;
	meshDesc.vertexFormat = VertexFormat::POSITION | VertexFormat::NORMAL | VertexFormat::TANGENT | VertexFormat::TEXCOORD0;
	meshDesc.indicesNumber = ibDesc.numElements;
	meshDesc.indexFormat = IndexFormat::UINT16_INDEX;
	meshDesc.topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	_mesh = new Mesh(meshDesc, vertexBufferId, indexBufferId, resourceManager);
}

void Common::Logic::SceneEntity::VegatationSystem::CreateConstantBuffers(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, Graphics::Resources::ResourceManager* resourceManager,
	const VegetationSystemDesc& desc)
{
	BufferDesc bufferDesc{};
	bufferDesc.data.resize(sizeof(MutableConstants), 0u);
	bufferDesc.flag = BufferFlag::IS_CONSTANT_DYNAMIC;

	mutableConstantsId = resourceManager->CreateBufferResource(device, commandList, BufferResourceType::CONSTANT_BUFFER, bufferDesc);

	auto mutableConstantsResource = resourceManager->GetResource<ConstantBuffer>(mutableConstantsId);
	mutableConstantsBuffer = reinterpret_cast<MutableConstants*>(mutableConstantsResource->resourceCPUAddress);
	mutableConstantsBuffer->viewProjection = _camera->GetViewProjection();
	mutableConstantsBuffer->cameraPosition = _camera->GetPosition();
	mutableConstantsBuffer->time = 0.0f;
	mutableConstantsBuffer->atlasElementSize.x = 1.0f / desc.atlasRows;
	mutableConstantsBuffer->atlasElementSize.y = 1.0f / desc.atlasColumns;
	mutableConstantsBuffer->perlinNoiseTiling = desc.perlinNoiseTiling;
	mutableConstantsBuffer->windDirection = *desc.windDirection;
	mutableConstantsBuffer->windStrength = *desc.windStrength;
	mutableConstantsBuffer->zNear = LightingSystem::SHADOW_MAP_Z_NEAR;
	mutableConstantsBuffer->zFar = LightingSystem::SHADOW_MAP_Z_FAR;
	mutableConstantsBuffer->padding = {};

	windDirection = desc.windDirection;
	windStrength = desc.windStrength;
}

void Common::Logic::SceneEntity::VegatationSystem::CreateBuffers(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, Graphics::Resources::ResourceManager* resourceManager,
	const VegetationSystemDesc& desc)
{
	uint32_t smallGrassInstanceNumber = QUADS_PER_SMALL_GRASS * desc.smallGrassNumber;
	uint32_t mediumGrassInstanceNumber = QUADS_PER_MEDIUM_GRASS * desc.mediumGrassNumber;
	uint32_t largeGrassInstanceNumber = QUADS_PER_LARGE_GRASS * desc.largeGrassNumber;
	uint32_t grassInstanceNumber = smallGrassInstanceNumber + mediumGrassInstanceNumber + largeGrassInstanceNumber;
	uint32_t maxCapNumber = desc.smallGrassNumber + desc.mediumGrassNumber + desc.largeGrassNumber;

	uint32_t largeGrassInstanceStartIndex = smallGrassInstanceNumber + mediumGrassInstanceNumber;

	instancesNumber = grassInstanceNumber + maxCapNumber;

	BufferDesc bufferDesc{};
	bufferDesc.dataStride = sizeof(Vegetation);
	
	if (std::filesystem::exists(desc.vegetationCacheFileName))
	{
		LoadCache(desc.vegetationCacheFileName, bufferDesc.data);
		instancesNumber = static_cast<uint32_t>(bufferDesc.data.size() / bufferDesc.dataStride);
	}
	else
	{
		TextureDesc vegetationMapDesc{};
		DDSLoader::Load(desc.vegetationMapFileName, vegetationMapDesc);

		bufferDesc.data.resize(static_cast<size_t>(bufferDesc.dataStride) * instancesNumber);

		XMUBYTE4 mask{};
		mask.z = 255u;

		uint32_t capOffset = grassInstanceNumber;
		uint32_t capNumber = 0u;

		VegetationBufferDesc vbDesc
		{
			bufferDesc.data.data(),
			vegetationMapDesc,
			desc,
			smallGrassInstanceNumber,
			0u,
			capOffset,
			QUADS_PER_SMALL_GRASS,
			mask,
			desc.smallGrassSizeMin,
			desc.smallGrassSizeMax,
			SMALL_GRASS_TILT_AMPLITUDE
		};

		FillVegetationBuffer(vbDesc, capNumber);
		
		vbDesc.elementNumber = mediumGrassInstanceNumber;
		vbDesc.offset = smallGrassInstanceNumber;
		vbDesc.quadNumber = QUADS_PER_MEDIUM_GRASS;
		vbDesc.mask.z = 0u;
		vbDesc.mask.y = 255u;
		vbDesc.grassSizeMin = desc.mediumGrassSizeMin;
		vbDesc.grassSizeMax = desc.mediumGrassSizeMax;
		vbDesc.tiltAmplitude = MEDIUM_GRASS_TILT_AMPLITUDE;

		FillVegetationBuffer(vbDesc, capNumber);
		
		vbDesc.elementNumber = largeGrassInstanceNumber;
		vbDesc.offset = largeGrassInstanceStartIndex;
		vbDesc.quadNumber = QUADS_PER_LARGE_GRASS;
		vbDesc.mask.y = 0u;
		vbDesc.mask.x = 255u;
		vbDesc.grassSizeMin = desc.largeGrassSizeMin;
		vbDesc.grassSizeMax = desc.largeGrassSizeMax;
		vbDesc.tiltAmplitude = LARGE_GRASS_TILT_AMPLITUDE;

		FillVegetationBuffer(vbDesc, capNumber);
		
		instancesNumber = grassInstanceNumber + capNumber;

		bufferDesc.data.resize(static_cast<size_t>(bufferDesc.dataStride) * instancesNumber);

		SaveCache(desc.vegetationCacheFileName, bufferDesc.data.data(), bufferDesc.data.size());
	}

	bufferDesc.numElements = static_cast<uint32_t>(bufferDesc.data.size() / bufferDesc.dataStride);

	vegetationBufferId = resourceManager->CreateBufferResource(device, commandList, BufferResourceType::BUFFER, bufferDesc);
}

void Common::Logic::SceneEntity::VegatationSystem::LoadShaders(ID3D12Device* device,
	Graphics::Resources::ResourceManager* resourceManager, const VegetationSystemDesc& desc)
{
	std::wstringstream lightMatricesNumberStream;
	lightMatricesNumberStream << desc.lightMatricesNumber;
	std::wstring lightMatricesNumberString;
	lightMatricesNumberStream >> lightMatricesNumberString;

	std::vector<DxcDefine> defines;
	defines.push_back({ L"LIGHT_MATRICES_NUMBER", lightMatricesNumberString.c_str() });

	std::vector<DxcDefine> definesDepthPrepass;
	definesDepthPrepass.push_back({ L"DEPTH_PREPASS", nullptr });

	vegetationVSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\VegetationVS.hlsl",
		ShaderType::VERTEX_SHADER, ShaderVersion::SM_6_5);

	vegetationPSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\VegetationPS.hlsl",
		ShaderType::PIXEL_SHADER, ShaderVersion::SM_6_5, *desc.lightDefines);

	vegetationDepthPrepassVSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\VegetationDepthPassVS.hlsl",
		ShaderType::VERTEX_SHADER, ShaderVersion::SM_6_5, definesDepthPrepass);

	vegetationDepthPassVSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\VegetationDepthPassVS.hlsl",
		ShaderType::VERTEX_SHADER, ShaderVersion::SM_6_5, defines);

	vegetationDepthPassPSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\VegetationDepthPassPS.hlsl",
		ShaderType::PIXEL_SHADER, ShaderVersion::SM_6_5);
	
	vegetationDepthCubePassVSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\VegetationDepthCubePassVS.hlsl",
		ShaderType::VERTEX_SHADER, ShaderVersion::SM_6_5);

	vegetationDepthCubePassGSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\VegetationDepthCubePassGS.hlsl",
		ShaderType::GEOMETRY_SHADER, ShaderVersion::SM_6_5, defines);

	vegetationDepthCubePassPSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\VegetationDepthCubePassPS.hlsl",
		ShaderType::PIXEL_SHADER, ShaderVersion::SM_6_5);
}

void Common::Logic::SceneEntity::VegatationSystem::LoadTextures(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, Graphics::Resources::ResourceManager* resourceManager,
	const VegetationSystemDesc& desc)
{
	TextureDesc textureDesc{};
	DDSLoader::Load(desc.albedoMapFileName, textureDesc);
	albedoMapId = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);
	DDSLoader::Load(desc.normalMapFileName, textureDesc);
	normalMapId = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);
}

void Common::Logic::SceneEntity::VegatationSystem::CreateMaterials(ID3D12Device* device,
	Graphics::Resources::ResourceManager* resourceManager, const VegetationSystemDesc& desc)
{
	auto lightConstantBufferResource = resourceManager->GetResource<ConstantBuffer>(lightConstantBufferId);
	auto mutableConstantsResource = resourceManager->GetResource<ConstantBuffer>(mutableConstantsId);
	auto vegetationBufferResource = resourceManager->GetResource<Buffer>(vegetationBufferId);
	auto perlinNoiseResource = resourceManager->GetResource<Texture>(desc.perlinNoiseId);
	auto albedoResource = resourceManager->GetResource<Texture>(albedoMapId);
	auto normalResource = resourceManager->GetResource<Texture>(normalMapId);
	auto samplerLinearResource = resourceManager->GetDefaultSampler(device,
		Graphics::DefaultFilterSetup::FILTER_TRILINEAR_WRAP);
	auto samplerShadowResource = resourceManager->GetDefaultSampler(device,
		Graphics::DefaultFilterSetup::FILTER_COMPARISON_POINT_CLAMP,
		Graphics::DefaultFilterComparisonFunc::COMPARISON_LESS_EQUAL);

	auto vegetationVS = resourceManager->GetResource<Shader>(vegetationVSId);
	auto vegetationPS = resourceManager->GetResource<Shader>(vegetationPSId);

	MaterialBuilder materialBuilder{};
	materialBuilder.SetConstantBuffer(0u, lightConstantBufferResource->resourceGPUAddress, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetConstantBuffer(1u, mutableConstantsResource->resourceGPUAddress);
	materialBuilder.SetBuffer(0u, vegetationBufferResource->resourceGPUAddress, D3D12_SHADER_VISIBILITY_VERTEX);
	materialBuilder.SetTexture(1u, perlinNoiseResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_VERTEX);
	materialBuilder.SetTexture(2u, albedoResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetTexture(3u, normalResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);

	for (uint32_t shadowMapIndex = 0u; shadowMapIndex < desc.shadowMapIds.size(); shadowMapIndex++)
	{
		auto shadowMapResource = resourceManager->GetResource<Texture>(desc.shadowMapIds[shadowMapIndex]);
		materialBuilder.SetTexture(4u + shadowMapIndex, shadowMapResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	}

	materialBuilder.SetSampler(0u, samplerLinearResource->samplerDescriptor.gpuDescriptor);
	materialBuilder.SetSampler(1u, samplerShadowResource->samplerDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetDepthBias(-7.5f);
	materialBuilder.SetCullMode(D3D12_CULL_MODE_BACK);
	materialBuilder.SetBlendMode(Graphics::DirectX12Utilities::CreateBlendDesc(Graphics::DefaultBlendSetup::BLEND_OPAQUE));
	materialBuilder.SetDepthStencilFormat(32u, true, true);
	materialBuilder.SetRenderTargetFormat(0u, DXGI_FORMAT_R16G16B16A16_FLOAT);
	materialBuilder.SetGeometryFormat(_mesh->GetDesc().vertexFormat, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	materialBuilder.SetVertexShader(vegetationVS->bytecode);
	materialBuilder.SetPixelShader(vegetationPS->bytecode);

	_material = materialBuilder.ComposeStandard(device);

	auto lightMatricesConstantBufferResource = resourceManager->GetResource<ConstantBuffer>(lightMatricesConstantBufferId);

	auto vegetationDepthPrepassVS = resourceManager->GetResource<Shader>(vegetationDepthPrepassVSId);
	auto vegetationDepthPassPS = resourceManager->GetResource<Shader>(vegetationDepthPassPSId);

	materialBuilder.SetConstantBuffer(0u, mutableConstantsResource->resourceGPUAddress, D3D12_SHADER_VISIBILITY_VERTEX);
	materialBuilder.SetBuffer(0u, vegetationBufferResource->resourceGPUAddress, D3D12_SHADER_VISIBILITY_VERTEX);
	materialBuilder.SetTexture(1u, perlinNoiseResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_VERTEX);
	materialBuilder.SetTexture(2u, normalResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetSampler(0u, samplerLinearResource->samplerDescriptor.gpuDescriptor);
	materialBuilder.SetCullMode(D3D12_CULL_MODE_BACK);
	materialBuilder.SetBlendMode(Graphics::DirectX12Utilities::CreateBlendDesc(Graphics::DefaultBlendSetup::BLEND_OPAQUE));
	materialBuilder.SetDepthStencilFormat(32u, true);
	materialBuilder.SetGeometryFormat(_mesh->GetDesc().vertexFormat, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	materialBuilder.SetVertexShader(vegetationDepthPrepassVS->bytecode);
	materialBuilder.SetPixelShader(vegetationDepthPassPS->bytecode);

	materialDepthPrepass = materialBuilder.ComposeStandard(device);
	
	if (desc.hasDepthPass)
	{
		auto lightMatricesConstantBufferResource = resourceManager->GetResource<ConstantBuffer>(lightMatricesConstantBufferId);
		
		auto vegetationDepthPassVS = resourceManager->GetResource<Shader>(vegetationDepthPassVSId);
		
		materialBuilder.SetRootConstants(0u, 1u, D3D12_SHADER_VISIBILITY_VERTEX);
		materialBuilder.SetConstantBuffer(1u, mutableConstantsResource->resourceGPUAddress, D3D12_SHADER_VISIBILITY_VERTEX);
		materialBuilder.SetConstantBuffer(2u, lightMatricesConstantBufferResource->resourceGPUAddress, D3D12_SHADER_VISIBILITY_VERTEX);
		materialBuilder.SetBuffer(0u, vegetationBufferResource->resourceGPUAddress, D3D12_SHADER_VISIBILITY_VERTEX);
		materialBuilder.SetTexture(1u, perlinNoiseResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_VERTEX);
		materialBuilder.SetTexture(2u, normalResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
		materialBuilder.SetSampler(0u, samplerLinearResource->samplerDescriptor.gpuDescriptor);
		materialBuilder.SetCullMode(D3D12_CULL_MODE_BACK);
		materialBuilder.SetBlendMode(Graphics::DirectX12Utilities::CreateBlendDesc(Graphics::DefaultBlendSetup::BLEND_OPAQUE));
		materialBuilder.SetDepthStencilFormat(32u, true);
		materialBuilder.SetGeometryFormat(_mesh->GetDesc().vertexFormat, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		materialBuilder.SetVertexShader(vegetationDepthPassVS->bytecode);
		materialBuilder.SetPixelShader(vegetationDepthPassPS->bytecode);

		materialDepthPass = materialBuilder.ComposeStandard(device);
	}

	if (desc.hasDepthCubePass)
	{
		auto lightMatricesConstantBufferResource = resourceManager->GetResource<ConstantBuffer>(lightMatricesConstantBufferId);

		auto vegetationDepthCubePassVS = resourceManager->GetResource<Shader>(vegetationDepthCubePassVSId);
		auto vegetationDepthCubePassGS = resourceManager->GetResource<Shader>(vegetationDepthCubePassGSId);
		auto vegetationDepthCubePassPS = resourceManager->GetResource<Shader>(vegetationDepthCubePassPSId);

		materialBuilder.SetRootConstants(0u, 1u, D3D12_SHADER_VISIBILITY_GEOMETRY);
		materialBuilder.SetConstantBuffer(1u, mutableConstantsResource->resourceGPUAddress);
		materialBuilder.SetConstantBuffer(2u, lightMatricesConstantBufferResource->resourceGPUAddress, D3D12_SHADER_VISIBILITY_GEOMETRY);
		materialBuilder.SetBuffer(0u, vegetationBufferResource->resourceGPUAddress, D3D12_SHADER_VISIBILITY_VERTEX);
		materialBuilder.SetTexture(1u, perlinNoiseResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_VERTEX);
		materialBuilder.SetTexture(2u, normalResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
		materialBuilder.SetSampler(0u, samplerLinearResource->samplerDescriptor.gpuDescriptor);
		materialBuilder.SetCullMode(D3D12_CULL_MODE_BACK);
		materialBuilder.SetBlendMode(Graphics::DirectX12Utilities::CreateBlendDesc(Graphics::DefaultBlendSetup::BLEND_OPAQUE));
		materialBuilder.SetDepthStencilFormat(32u, true);
		materialBuilder.SetGeometryFormat(_mesh->GetDesc().vertexFormat, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		materialBuilder.SetVertexShader(vegetationDepthCubePassVS->bytecode);
		materialBuilder.SetGeometryShader(vegetationDepthCubePassGS->bytecode);
		materialBuilder.SetPixelShader(vegetationDepthCubePassPS->bytecode);

		materialDepthCubePass = materialBuilder.ComposeStandard(device);
	}
}

void Common::Logic::SceneEntity::VegatationSystem::FillVegetationBuffer(const VegetationBufferDesc& desc,
	uint32_t& capNumber)
{
	auto vegetations = reinterpret_cast<Vegetation*>(desc.buffer);
	
	auto origin = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	auto identityQuaternion = XMQuaternionIdentity();
	auto capQuaternion = XMQuaternionRotationAxis(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f),
		static_cast<float>(std::numbers::pi * 0.5f));

	auto upVector = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

	floatN position{};
	floatN upVectorTarget{};
	floatN rolling{};
	floatN rotation{};
	float2 atlasElementOffset{};
	float2 capAtlasElementOffset{};
	bool hasCap{};

	for (uint32_t grassIndex = 0u; grassIndex < desc.elementNumber; grassIndex++)
	{
		capAtlasElementOffset = {};
		hasCap = false;

		auto randomX = Utilities::Random(randomEngine);
		auto randomY = Utilities::Random(randomEngine);
		auto randomZ = Utilities::Random(randomEngine);

		float3 grassSize{};
		grassSize.x = std::lerp(desc.grassSizeMin.x, desc.grassSizeMax.x, randomX);
		grassSize.y = std::lerp(desc.grassSizeMin.y, desc.grassSizeMax.y, randomY);
		grassSize.z = std::lerp(desc.grassSizeMin.z, desc.grassSizeMax.z, randomZ);
		auto scale = XMLoadFloat3(&grassSize);
		
		auto& vegetation = vegetations[grassIndex + desc.offset];

		if (grassIndex % desc.quadNumber == 0u)
		{
			GetRandomData(desc, position, upVectorTarget, atlasElementOffset, capAtlasElementOffset, hasCap);
			rotation = CalculateRotation(upVector, upVectorTarget);
			rolling = XMQuaternionRotationAxis(upVectorTarget, static_cast<float>(std::numbers::pi * 2.0 / 3.0));

			if (hasCap)
			{
				auto maxScale = std::max(scale.m128_f32[0], std::max(scale.m128_f32[1], scale.m128_f32[2]));
				auto capScale = XMVectorSet(maxScale, maxScale, maxScale, 1.0f);
				auto capPosition = position;
				capPosition.m128_f32[2] += grassSize.z * 0.25f;
				auto capRotation = XMQuaternionMultiply(capQuaternion, rotation);
				
				auto& cap = vegetations[desc.capOffset + capNumber];

				cap.world = XMMatrixTransformation(origin, identityQuaternion, capScale, origin, capRotation, capPosition);
				cap.atlasElementOffset = capAtlasElementOffset;
				cap.tiltAmplitude = desc.tiltAmplitude;
				cap.height = -grassSize.z;

				capNumber++;
			}
		}
		else
			rotation = XMQuaternionMultiply(rotation, rolling);

		vegetation.world = XMMatrixTransformation(origin, rotation, scale, origin, rotation, position);
		vegetation.atlasElementOffset = atlasElementOffset;
		vegetation.tiltAmplitude = desc.tiltAmplitude;
		vegetation.height = grassSize.z;
	}
}

void Common::Logic::SceneEntity::VegatationSystem::GetRandomData(const VegetationBufferDesc& desc,
	floatN& position, floatN& upVector, float2& atlasOffset, float2& capAtlasOffset, bool& hasCap)
{
	auto mapSize = static_cast<uint32_t>(desc.vegetationMapDesc.width * desc.vegetationMapDesc.height);
	auto random = Utilities::Random(randomEngine);

	auto absoluteUpVector = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

	auto startIndex = std::min(static_cast<uint32_t>(random * mapSize), mapSize - 1u);

	auto vegetationDataPtr = reinterpret_cast<const XMUBYTE4*>(desc.vegetationMapDesc.data.data());

	auto terrain = desc.vegetationSystemDesc.terrain;
	auto& minCorner = terrain->GetMinCorner();
	auto& size = terrain->GetSize();

	for (uint32_t pixelIndex = 0; pixelIndex < mapSize; pixelIndex++)
	{
		auto fetchedIndex = (pixelIndex * 32u + startIndex) % mapSize;

		auto vegetationDataV = *(vegetationDataPtr + fetchedIndex);

		int32_t vegetationData = (vegetationDataV.x & desc.mask.x) | (vegetationDataV.y & desc.mask.y) |
			(vegetationDataV.z & desc.mask.z);

		int32_t capIndex = vegetationDataV.w;

		if (vegetationData > 0)
		{
			atlasOffset.x = ((vegetationData - 1) % desc.vegetationSystemDesc.atlasRows) /
				static_cast<float>(desc.vegetationSystemDesc.atlasRows);

			atlasOffset.y = static_cast<float>((vegetationData - 1) / desc.vegetationSystemDesc.atlasRows) /
				desc.vegetationSystemDesc.atlasColumns;

			if (capIndex > 0)
			{
				capAtlasOffset.x = ((capIndex - 1) % desc.vegetationSystemDesc.atlasRows) /
					static_cast<float>(desc.vegetationSystemDesc.atlasRows);

				capAtlasOffset.y = static_cast<float>((capIndex - 1) / desc.vegetationSystemDesc.atlasRows) /
					desc.vegetationSystemDesc.atlasColumns;

				hasCap = true;
			}

			auto xCoeff = static_cast<float>(fetchedIndex % static_cast<uint32_t>(desc.vegetationMapDesc.width));
			xCoeff /= desc.vegetationMapDesc.width;

			auto yCoeff = static_cast<float>(fetchedIndex / static_cast<uint32_t>(desc.vegetationMapDesc.width));
			yCoeff /= desc.vegetationMapDesc.height;

			auto planarPosition = float2(minCorner.x + size.x * xCoeff, minCorner.y + size.y * yCoeff);
			auto height = terrain->GetHeight(planarPosition);
			auto normal = terrain->GetNormal(planarPosition);

			auto normalV = XMLoadFloat3(&normal);
			normalV = XMVectorLerp(normalV, absoluteUpVector, 0.75f);
			normalV = XMVector3Normalize(normalV);
			
			XMStoreFloat3(&normal, normalV);

			if (XMVector3Dot(normalV, normalV).m128_f32[0u] < std::numeric_limits<float>::epsilon())
				normal = float3(0.0f, 0.0f, 1.0f);

			position = XMVectorSet(planarPosition.x, planarPosition.y, height, 0.0f);
			upVector = XMLoadFloat3(&normal);

			return;
		}
	}

	position = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	upVector = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
}

floatN Common::Logic::SceneEntity::VegatationSystem::CalculateRotation(const floatN& upVector, const floatN& upVectorTarget)
{
	auto rotation = XMQuaternionRotationAxis(upVector,
		Utilities::Random(randomEngine) * static_cast<float>(std::numbers::pi * 2.0));
	
	auto angle = XMVector3Dot(upVectorTarget, upVector);
	
	if (std::abs(angle.m128_f32[0u] - 1.0f) < std::numeric_limits<float>::epsilon())
	{
		auto rotationAxis = XMVector3Cross(upVectorTarget, upVector);

		if (XMVector3Dot(rotationAxis, rotationAxis).m128_f32[0u] < std::numeric_limits<float>::epsilon())
			rotationAxis = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

		auto pitch = XMQuaternionRotationAxis(rotationAxis, angle.m128_f32[0u]);

		rotation = XMQuaternionMultiply(rotation, pitch);
	}

	return rotation;
}

Common::Logic::SceneEntity::VegatationSystem::GrassVertex Common::Logic::SceneEntity::VegatationSystem::SetVertex(const float3& position,
	uint64_t normal, uint64_t tangent, const DirectX::PackedVector::XMHALF2& texCoord)
{
	GrassVertex vertex{};
	vertex.position = position;
	vertex.normalXY = static_cast<uint32_t>(normal & 0xFFFFFFFFull);
	vertex.normalZW = static_cast<uint32_t>((normal & 0xFFFFFFFF00000000ull) >> 32u);
	vertex.tangentXY = static_cast<uint32_t>(tangent & 0xFFFFFFFFull);
	vertex.tangentZW = static_cast<uint32_t>((tangent & 0xFFFFFFFF00000000ull) >> 32u);
	vertex.texCoord = texCoord;

	return vertex;
}

void Common::Logic::SceneEntity::VegatationSystem::LoadCache(const std::filesystem::path& fileName,
	std::vector<uint8_t>& buffer)
{
	std::ifstream vegetationFile(fileName, std::ios::binary);

	uint32_t bufferSize{};
	vegetationFile.read(reinterpret_cast<char*>(&bufferSize), sizeof(uint32_t));

	buffer.resize(bufferSize);

	vegetationFile.read(reinterpret_cast<char*>(buffer.data()), bufferSize);
}

void Common::Logic::SceneEntity::VegatationSystem::SaveCache(const std::filesystem::path& fileName,
	const uint8_t* buffer, size_t size)
{
	auto _size = size;

	std::ofstream vegetationFile(fileName, std::ios::binary);
	vegetationFile.write(reinterpret_cast<const char*>(&_size), sizeof(uint32_t));
	vegetationFile.write(reinterpret_cast<const char*>(buffer), size);
}
