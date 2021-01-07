#pragma once
#include <DirectXMath.h>

#include "DXCore.h"
#include "Transform.h"

class Camera
{
public:
    Camera(float x, float y, float z, float aspectRatio, float fov, float nPlane, float fPlane, float moveSpeed, float mouseLookSpeed);

    // getters
    DirectX::XMFLOAT4X4 GetViewMatrix();
    DirectX::XMFLOAT4X4 GetProjectionMatrix();
    Transform* GetTransform();

    // methods
    void UpdateViewMatrix();
    void UpdateProjectionMatrix(float aspectRatio);
    void Update(float dt, HWND windowHandle, int controlMode);

private:
    DirectX::XMFLOAT4X4 viewMatrix;
    DirectX::XMFLOAT4X4 projMatrix;
    Transform transform;
    POINT mInput;
    float fieldOfView;
    float nearPlane;
    float farPlane;
    float movementSpeed;
    float mLookSpeed;
};

