#pragma once
#include <d3d11.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include <fstream>
#include <vector>
#include "Vertex.h"

class Mesh
{
public:
    Mesh(Vertex* vertices, int numVert, unsigned int * indices, int nIndices, Microsoft::WRL::ComPtr<ID3D11Device> device);
    Mesh(const char* fileName, Microsoft::WRL::ComPtr<ID3D11Device> device);
    ~Mesh();

    // public methods
    Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer();
    Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer();
    int GetIndexCount();

private: 
    // private vars
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer = 0;
    Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer = 0;
    int numIndices = 0;

    // private methods
    void CalculateTangents(Vertex* verts, int numVerts, unsigned int* indices, int numIndices);
    void CreateVertexBuffers(Vertex* vertices, int numVert, UINT* indices, int nIndices, Microsoft::WRL::ComPtr<ID3D11Device> device);

};

