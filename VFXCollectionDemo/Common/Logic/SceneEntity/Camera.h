#pragma once

#include "../../../Includes.h"
#include "../../../Graphics/DirectX12Includes.h"

namespace Common::Logic::SceneEntity
{
	class Camera
	{
	public:
		Camera(const float3& position, const float3& lookAt, const float3& upVector,
			float fovY, float aspectRatio, float zNear, float zFar);
		~Camera();

		void Update(const float3& position, const float3& lookAt, const float3& upVector);
		void UpdateProjection(float fovY, float aspectRatio, float zNear, float zFar);

		const float4x4& GetView();
		const float4x4& GetProjection();
		const float4x4& GetViewProjection();

		const float4x4& GetInvView();
		const float4x4& GetInvProjection();
		const float4x4& GetInvViewProjection();

	private:
		Camera() = delete;

		float4x4 view;
		float4x4 projection;
		float4x4 viewProjection;

		float4x4 invView;
		float4x4 invProjection;
		float4x4 invViewProjection;
	};
}
