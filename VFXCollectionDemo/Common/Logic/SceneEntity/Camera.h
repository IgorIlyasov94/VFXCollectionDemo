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

		void AddJitter(const float2& jitter, const float2& renderSize);

		const float4x4& GetView() const;
		const float4x4& GetProjection() const;
		const float4x4& GetViewProjection() const;

		const float4x4& GetInvView() const;
		const float4x4& GetInvProjection() const;
		const float4x4& GetInvViewProjection() const;

		const float3& GetPosition() const;
		const float3& GetDirection() const;

		const float GetZNear() const;
		const float GetZFar() const;
		const float GetFovY() const;
		const float GetAspectRatio() const;

	private:
		Camera() = delete;

		float4x4 view;
		float4x4 projection;
		float4x4 viewProjection;

		float4x4 invView;
		float4x4 invProjection;
		float4x4 invViewProjection;

		float3 _position;
		float3 direction;

		float4x4 baseProjection;

		float _zNear;
		float _zFar;
		float _fovY;
		float _aspectRatio;
	};
}
