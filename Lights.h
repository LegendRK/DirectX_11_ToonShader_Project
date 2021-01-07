#pragma once
#include <DirectXMath.h>
#define MAX_LIGHTS          128
#define TYPE_DIRECTIONAL    0
#define TYPE_POINT          1
#define TYPE_SPOT           2

struct DirectionalLight {
    DirectX::XMFLOAT3 ambientColor;     // 1st boundary 12 + 4
    float padding1;
    DirectX::XMFLOAT3 diffuseColor;     // 2nd boundary 12 + 4
    float padding2;
    DirectX::XMFLOAT3 direction;        // 3rd boundary 12
};

struct PointLight {
    DirectX::XMFLOAT3 ambientColor;     // 1st boundary 12 + 4
    float radius;
    DirectX::XMFLOAT3 diffuseColor;     // 2nd boundary 12 + 4
    float padding;
    DirectX::XMFLOAT3 position;         // 3rd boundary 12 
};

struct SpotLight {
    DirectX::XMFLOAT3 ambientColor;     // 1st boundary 12 + 4
    float spotPower;
    DirectX::XMFLOAT3 diffuseColor;     // 2nd boundary 12 + 4
    float radius;
    DirectX::XMFLOAT3 position;         // 3rd boundary 12 + 4
    float padding;
    DirectX::XMFLOAT3 direction;        // 4th boundary 12
};

struct Light {
    int type;
    DirectX::XMFLOAT3 direction;
    // -------------
    float radius;
    DirectX::XMFLOAT3 position;
    // -------------
    float intensity;
    DirectX::XMFLOAT3 diffuseColor;
    // -------------
    float spotPower;
    DirectX::XMFLOAT3 ambientColor;
    // -------------
    int enabled;
    DirectX::XMFLOAT3 padding;
    // -------------
};