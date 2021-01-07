#pragma once

#include "DXCore.h"
#include "Mesh.h"
#include "Transform.h"
#include "BufferStructs.h"
#include "Camera.h"
#include "Material.h"
#include <DirectXMath.h>

class Entity
{
public:
    Entity(Mesh* p_Mesh, Material* p_Mat);

    // getters
    Mesh* GetMesh();
    Material* GetMaterial();
    Transform* GetTransform();

    // methods
    // void Draw(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, Microsoft::WRL::ComPtr<ID3D11Buffer> vsConstantBuffer, Camera* camera);
    void Draw(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, Camera* camera);

private:
    Transform transform;
    Mesh* mesh;
    Material* mat;
};

