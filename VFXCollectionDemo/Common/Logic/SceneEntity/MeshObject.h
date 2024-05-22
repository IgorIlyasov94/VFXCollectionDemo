#pragma once

#include "../../../Includes.h"
#include "IDrawable.h"
#include "../../../Graphics/Assets/Material.h"
#include "../../../Graphics/Assets/Mesh.h"

namespace Common::Logic::SceneEntity
{
	class MeshObject : public IDrawable
	{
	public:
		MeshObject(Graphics::Assets::Mesh* mesh, Graphics::Assets::Material* material);
		~MeshObject() override;

		void Draw(ID3D12GraphicsCommandList* commandList) override;

	private:
		MeshObject() = delete;

		Graphics::Assets::Mesh* _mesh;
		Graphics::Assets::Material* _material;
	};
}
