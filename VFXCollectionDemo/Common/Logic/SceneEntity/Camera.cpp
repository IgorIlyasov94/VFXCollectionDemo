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
	_position = position;
	auto _positionV = XMLoadFloat3(&position);
	auto _lookAt = XMLoadFloat3(&lookAt);
	auto _upVector = XMLoadFloat3(&upVector);

	view = XMMatrixLookAtRH(_positionV, _lookAt, _upVector);
	viewProjection = view * projection;
	invView = XMMatrixInverse(nullptr, view);
	invViewProjection = XMMatrixInverse(nullptr, viewProjection);

	auto _direction = _lookAt - _positionV;
	_direction = XMVector3Normalize(_direction);

	XMStoreFloat3(&direction, _direction);
}

void Common::Logic::SceneEntity::Camera::UpdateProjection(float fovY, float aspectRatio, float zNear, float zFar)
{
	_zNear = zNear;
	_zFar = zFar;
	_fovY = fovY;
	_aspectRatio = aspectRatio;

	projection = XMMatrixPerspectiveFovRH(fovY, aspectRatio, zNear, zFar);
	invProjection = XMMatrixInverse(nullptr, projection);

	viewProjection = view * projection;
	invViewProjection = XMMatrixInverse(nullptr, viewProjection);

	baseProjection = projection;
}

void Common::Logic::SceneEntity::Camera::AddJitter(const float2& jitter, const float2& renderSize)
{
	auto jitterVector = XMVectorSet(2.0f * jitter.x / renderSize.x, -2.0f * jitter.y / renderSize.y, 0.0f, 1.0f);
	auto jitterMatrix = XMMatrixTranslationFromVector(jitterVector);

	projection = jitterMatrix * baseProjection;
}

const float4x4& Common::Logic::SceneEntity::Camera::GetView() const
{
	return view;
}

const float4x4& Common::Logic::SceneEntity::Camera::GetProjection() const
{
	return projection;
}

const float4x4& Common::Logic::SceneEntity::Camera::GetViewProjection() const
{
	return viewProjection;
}

const float4x4& Common::Logic::SceneEntity::Camera::GetInvView() const
{
	return invView;
}

const float4x4& Common::Logic::SceneEntity::Camera::GetInvProjection() const
{
	return invProjection;
}

const float4x4& Common::Logic::SceneEntity::Camera::GetInvViewProjection() const
{
	return invViewProjection;
}

const float3& Common::Logic::SceneEntity::Camera::GetPosition() const
{
	return _position;
}

const float3& Common::Logic::SceneEntity::Camera::GetDirection() const
{
	return direction;
}

const float Common::Logic::SceneEntity::Camera::GetZNear() const
{
	return _zNear;
}

const float Common::Logic::SceneEntity::Camera::GetZFar() const
{
	return _zFar;
}

const float Common::Logic::SceneEntity::Camera::GetFovY() const
{
	return _fovY;
}

const float Common::Logic::SceneEntity::Camera::GetAspectRatio() const
{
	return _aspectRatio;
}
