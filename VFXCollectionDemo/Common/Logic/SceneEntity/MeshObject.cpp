#include "MeshObject.h"

Common::Logic::SceneEntity::MeshObject::MeshObject(Graphics::Assets::Mesh* mesh, Graphics::Assets::Material* material,
	Graphics::Assets::Material* materialDepthPrepass, Graphics::Assets::Material* materialDepthPass,
	Graphics::Assets::Material* materialDepthCubePass)
	: _mesh(mesh), _material(material), _materialDepthPrepass(materialDepthPrepass),
	_materialDepthPass(materialDepthPass), _materialDepthCubePass(materialDepthCubePass)
{

}

Common::Logic::SceneEntity::MeshObject::~MeshObject()
{

}

void Common::Logic::SceneEntity::MeshObject::Update(float time, float deltaTime)
{

}

void Common::Logic::SceneEntity::MeshObject::OnCompute(ID3D12GraphicsCommandList* commandList)
{

}

void Common::Logic::SceneEntity::MeshObject::DrawDepthPrepass(ID3D12GraphicsCommandList* commandList)
{
	_materialDepthPrepass->Set(commandList);
	_mesh->Draw(commandList);
}

void Common::Logic::SceneEntity::MeshObject::DrawShadows(ID3D12GraphicsCommandList* commandList,
	uint32_t lightMatrixStartIndex)
{
	_materialDepthPass->Set(commandList);
	_mesh->Draw(commandList);
}

void Common::Logic::SceneEntity::MeshObject::DrawShadowsCube(ID3D12GraphicsCommandList* commandList,
	uint32_t lightMatrixStartIndex)
{
	_materialDepthCubePass->Set(commandList);
	_mesh->Draw(commandList);
}

void Common::Logic::SceneEntity::MeshObject::Draw(ID3D12GraphicsCommandList* commandList)
{
	_material->Set(commandList);
	_mesh->Draw(commandList);
}

void Common::Logic::SceneEntity::MeshObject::Release(Graphics::Resources::ResourceManager* resourceManager)
{

}
