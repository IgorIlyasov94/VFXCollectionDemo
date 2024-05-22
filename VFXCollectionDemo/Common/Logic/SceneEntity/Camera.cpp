#include "Camera.h"

using namespace DirectX;

Common::Logic::SceneEntity::Camera::Camera(const float3& position, const float3& lookAt, const float3& upVector,
	float fovY, float aspectRatio, float zNear, float zFar)
{
	UpdateProjection(fovY, aspectRatio, zNear, zFar);
	Update(position, lookAt, upVector);
}

Common::Logic::SceneEntity::Camera::~Camera()
{

}

void Common::Logic::SceneEntity::Camera::Update(const float3& position, const float3& lookAt, const float3& upVector)
{
	auto _position = XMLoadFloat3(&position);
	auto _lookAt = XMLoadFloat3(&lookAt);
	auto _upVector = XMLoadFloat3(&upVector);

	view = XMMatrixLookAtRH(_position, _lookAt, _upVector);
	viewProjection = view * projection;
	invView = XMMatrixInverse(nullptr, view);
	invViewProjection = XMMatrixInverse(nullptr, viewProjection);
}

void Common::Logic::SceneEntity::Camera::UpdateProjection(float fovY, float aspectRatio, float zNear, float zFar)
{
	projection = XMMatrixPerspectiveFovRH(fovY, aspectRatio, zNear, zFar);
	invProjection = XMMatrixInverse(nullptr, projection);

	viewProjection = view * projection;
	invViewProjection = XMMatrixInverse(nullptr, viewProjection);
}

const float4x4& Common::Logic::SceneEntity::Camera::GetView()
{
	return view;
}

const float4x4& Common::Logic::SceneEntity::Camera::GetProjection()
{
	return projection;
}

const float4x4& Common::Logic::SceneEntity::Camera::GetViewProjection()
{
	return viewProjection;
}

const float4x4& Common::Logic::SceneEntity::Camera::GetInvView()
{
	return invView;
}

const float4x4& Common::Logic::SceneEntity::Camera::GetInvProjection()
{
	return invProjection;
}

const float4x4& Common::Logic::SceneEntity::Camera::GetInvViewProjection()
{
	return invViewProjection;
}
