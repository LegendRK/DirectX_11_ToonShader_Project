#pragma once
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include <assert.h> 
#include "SimpleShader.h"
#include "Mesh.h"
#include "DXCore.h"
#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"
#include "Camera.h"

class SkyBox
{
public:
    // constructor
    SkyBox(Mesh* p_skyMesh, Microsoft::WRL::ComPtr<ID3D11SamplerState> p_skySS, Microsoft::WRL::ComPtr<ID3D11Device> device, std::wstring filePath, SimpleVertexShader* p_skyVS, SimplePixelShader* p_skyPS);
    SkyBox(Mesh* p_skyMesh, Microsoft::WRL::ComPtr<ID3D11SamplerState> p_skySS, Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, SimpleVertexShader* p_skyVS, SimplePixelShader* p_skyPS, std::wstring filePath_right, std::wstring filePath_left, std::wstring filePath_up, std::wstring filePath_down, std::wstring filePath_front, std::wstring filePath_back);
    
    // methods
    void Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, Camera* camera);

private:
    // vars
    Microsoft::WRL::ComPtr<ID3D11SamplerState> skySS;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> skySRV;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> skyDS;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> skyRS;

    Mesh* skyMesh;

    SimpleVertexShader* skyVS;
    SimplePixelShader* skyPS;

    //methods
    void SetGeneralParamaters(Mesh* p_skyMesh, Microsoft::WRL::ComPtr<ID3D11SamplerState> p_skySS, Microsoft::WRL::ComPtr<ID3D11Device> device, SimpleVertexShader* p_skyVS, SimplePixelShader* p_skyPS);
    Microsoft::WRL::ComPtr< ID3D11ShaderResourceView> CreateCubemap(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, const wchar_t* right, const wchar_t* left, const wchar_t* up, const wchar_t* down, const wchar_t* front, const wchar_t* back);
};

