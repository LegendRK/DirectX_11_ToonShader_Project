#include "Camera.h"

using namespace DirectX;

Camera::Camera(float x, float y, float z, float aspectRatio, float fov, float nPlane, float fPlane, float moveSpeed, float mouseLookSpeed)
{
    // set vars....
    transform.SetPosition(x, y, z);
    this->fieldOfView = fov;
    this->nearPlane = nPlane;
    this->farPlane = fPlane;
    this->movementSpeed = moveSpeed;
    this->mLookSpeed = mouseLookSpeed;

    // update projection/view
    UpdateProjectionMatrix(aspectRatio);
    UpdateViewMatrix();
}

// getters
DirectX::XMFLOAT4X4 Camera::GetViewMatrix() { return this->viewMatrix; }
DirectX::XMFLOAT4X4 Camera::GetProjectionMatrix() { return this->projMatrix; }
Transform* Camera::GetTransform() { return &transform; }


// methods
void Camera::UpdateViewMatrix()
{
    // get current facing direction
    XMFLOAT3 rotation = transform.GetPitchYawRoll();
    XMVECTOR direction = XMVector3Rotate(
        XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f),
        XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&rotation))
    );

    // update the view based on where we are looking
    XMMATRIX view = XMMatrixLookToLH(
        XMLoadFloat3(
            &transform.GetPosition()),
            direction,
            XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
    );

    XMStoreFloat4x4(&viewMatrix, view);
}

void Camera::UpdateProjectionMatrix(float aspectRatio)
{
    XMMATRIX proj = XMMatrixPerspectiveFovLH(fieldOfView, aspectRatio, nearPlane, farPlane);
    XMStoreFloat4x4(&projMatrix, proj);
}

void Camera::Update(float dt, HWND windowHandle, int controlMode)
{
    // keyboard input
    if (0 == controlMode)
    {
        if (GetAsyncKeyState('W') & 0x8000) { transform.MoveRelative(0.0f, 0.0f, movementSpeed * dt); }
        if (GetAsyncKeyState('A') & 0x8000) { transform.MoveRelative(-movementSpeed * dt, 0.0f, 0.0f); }
        if (GetAsyncKeyState('S') & 0x8000) { transform.MoveRelative(0.0f, 0.0f, -movementSpeed * dt); }
        if (GetAsyncKeyState('D') & 0x8000) { transform.MoveRelative(movementSpeed * dt, 0.0f, 0.0f); }
        if (GetAsyncKeyState(VK_SPACE) & 0x8000) { transform.MoveRelative(0.0f, movementSpeed * dt, 0.0f); }
        if (GetAsyncKeyState('X') & 0x8000) { transform.MoveRelative(0.0f, -movementSpeed * dt, 0.0f); }
    }

    // mouse input
    POINT mousePos = {};
    GetCursorPos(&mousePos);
    ScreenToClient(windowHandle, &mousePos);
    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) 
    { 
        
        float dx = dt * mLookSpeed * (mousePos.x - mInput.x);
        float dy = dt * mLookSpeed * (mousePos.y - mInput.y);
        transform.Rotate(dy, dx, 0.0f);
    }

    UpdateViewMatrix();

    // store the previous mouse input for next frame
    mInput = mousePos;
}
