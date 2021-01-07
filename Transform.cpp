#include "Transform.h"

using namespace DirectX;

Transform::Transform()
{
    // set defaults
    position = XMFLOAT3(0.0f, 0.0f, 0.0f);
    pitchYawRoll = XMFLOAT3(0.0f, 0.0f, 0.0f);
    scale = XMFLOAT3(1.0f, 1.0f, 1.0f);
    XMStoreFloat4x4(&worldMatrix, XMMatrixIdentity());

    isDirty = false;
}

// getters
XMFLOAT3 Transform::GetPosition() { return this->position; }
XMFLOAT3 Transform::GetPitchYawRoll() { return this->pitchYawRoll; }
XMFLOAT3 Transform::GetScale() { return this->scale; }
XMFLOAT4X4 Transform::GetWorldMatrix() 
{
    if (isDirty)
    {
        // recalc world matrix...
        XMMATRIX translationMatrix = XMMatrixTranslation(position.x, position.y, position.z);
        XMMATRIX scaleMatrix = XMMatrixScaling(scale.x, scale.y, scale.z);
        XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(pitchYawRoll.x, pitchYawRoll.y, pitchYawRoll.z);

        XMMATRIX world = scaleMatrix * rotationMatrix * translationMatrix;

        XMStoreFloat4x4(&worldMatrix, world);

        isDirty = false;
    }

    return this->worldMatrix;
}

DirectX::XMFLOAT4X4 Transform::GetInverseTransposeWorldMatrix()
{
    XMFLOAT4X4 invTransposeWorldMatrix;
    XMMATRIX world = XMLoadFloat4x4(&worldMatrix);
    XMMATRIX invWorld = XMMatrixInverse(nullptr, world);
    XMMATRIX invTransposeWorld = XMMatrixTranspose(invWorld);
    XMStoreFloat4x4(&invTransposeWorldMatrix, invTransposeWorld);

    return invTransposeWorldMatrix;
}

// setters
void Transform::SetPosition(float x, float y, float z) 
{
    position = XMFLOAT3(x, y, z);
    isDirty = true;
}

void Transform::SetPitchYawRoll(float pitch, float yaw, float roll)
{
    pitchYawRoll = XMFLOAT3(pitch, yaw, roll);
    isDirty = true;
}

void Transform::SetScale(float x, float y, float z)
{
    scale = XMFLOAT3(x, y, z);
    isDirty = true;
}

// transformers
void Transform::MoveAbsolute(float x, float y, float z)
{
    position = XMFLOAT3(position.x + x, position.y + y, position.z + z);
    isDirty = true;
}

void Transform::MoveRelative(float x, float y, float z)
{
    // create a vector of the input, create a quaternion of the current rotation
    XMVECTOR absDirection = XMVectorSet(x, y, z, 0);
    XMVECTOR rotation = XMQuaternionRotationRollPitchYawFromVector(XMVectorSet(pitchYawRoll.x, pitchYawRoll.y, pitchYawRoll.z, 0));

    // rotate the input by the current rotation quat, add to current position and store it.
    XMVECTOR relDirection = XMVector3Rotate(absDirection, rotation);
    XMVECTOR newPosition = XMLoadFloat3(&position) + relDirection;
    XMStoreFloat3(&position, newPosition);
}

void Transform::Rotate(float pitch, float yaw, float roll) 
{
    pitchYawRoll = XMFLOAT3(pitchYawRoll.x + pitch, pitchYawRoll.y + yaw, pitchYawRoll.z + roll);
    isDirty = true;
}

void Transform::Scale(float x, float y, float z)
{
    scale = XMFLOAT3(scale.x * x, scale.y * y, scale.z * z);
    isDirty = true;
}