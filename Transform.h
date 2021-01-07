#pragma once

#include <DirectXMath.h>

class Transform
{
public:
    Transform();

    // getters
    DirectX::XMFLOAT3 GetPosition();
    DirectX::XMFLOAT3 GetPitchYawRoll();
    DirectX::XMFLOAT3 GetScale();
    DirectX::XMFLOAT4X4 GetWorldMatrix();
    DirectX::XMFLOAT4X4 GetInverseTransposeWorldMatrix();

    // setters
    void SetPosition(float x, float y, float z);
    void SetPitchYawRoll(float pitch, float yaw, float roll);
    void SetScale(float x, float y, float z);

    // transformer methods
    void MoveAbsolute(float x, float y, float z);
    void MoveRelative(float x, float y, float z);
    void Rotate(float pitch, float yaw, float roll);
    void Scale(float x, float y, float z);

private:
    DirectX::XMFLOAT4X4 worldMatrix;
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 scale;
    DirectX::XMFLOAT3 pitchYawRoll;

    bool isDirty;
};

