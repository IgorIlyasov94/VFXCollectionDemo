#include "MeshObject.h"

Common::Logic::SceneEntity::MeshObject::MeshObject(Graphics::Assets::Mesh* mesh, Graphics::Assets::Material* material)
	: _mesh(mesh), _material(material)
{

}

Common::Logic::SceneEntity::MeshObject::~MeshObject()
{

}

void Common::Logic::SceneEntity::MeshObject::OnCompute(ID3D12GraphicsCommandList* commandList, float time, float deltaTime)
{

}

void Common::Logic::SceneEntity::MeshObject::Draw(ID3D12GraphicsCommandList* commandList, float time, float deltaTime)
{
	_material->Set(commandList);
	_mesh->Draw(commandList);
}

void Common::Logic::SceneEntity::MeshObject::Release(Graphics::Resources::ResourceManager* resourceManager)
{

}
