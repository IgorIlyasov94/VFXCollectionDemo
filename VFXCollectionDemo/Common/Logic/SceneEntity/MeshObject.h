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
		MeshObject(Graphics::Assets::Mesh* mesh, Graphics::Assets::Material* material,
			Graphics::Assets::Material* materialDepthPrepass, Graphics::Assets::Material* materialDepthPass,
			Graphics::Assets::Material* materialDepthCubePass);
		~MeshObject() override;

		void Update(float time, float deltaTime) override;

		void OnCompute(ID3D12GraphicsCommandList* commandList) override;

		void DrawDepthPrepass(ID3D12GraphicsCommandList* commandList) override;
		void DrawShadows(ID3D12GraphicsCommandList* commandList, uint32_t lightMatrixStartIndex) override;
		void DrawShadowsCube(ID3D12GraphicsCommandList* commandList, uint32_t lightMatrixStartIndex) override;
		void Draw(ID3D12GraphicsCommandList* commandList) override;

		void Release(Graphics::Resources::ResourceManager* resourceManager) override;

	private:
		MeshObject() = delete;

		Graphics::Assets::Mesh* _mesh;
		Graphics::Assets::Material* _material;
		Graphics::Assets::Material* _materialDepthPrepass;
		Graphics::Assets::Material* _materialDepthPass;
		Graphics::Assets::Material* _materialDepthCubePass;
	};
}
