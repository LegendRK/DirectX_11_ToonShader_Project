#pragma once
#include <DirectXMath.h>
#include "DXCore.h"
#include "SimpleShader.h"

class Material
{
public:
    //Material(DirectX::XMFLOAT4 cTint, Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel, Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex);
    Material(DirectX::XMFLOAT4 cTint, SimplePixelShader* pixel, SimpleVertexShader* vertex, float sIntensity, Microsoft::WRL::ComPtr<ID3D11SamplerState> p_sampler, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> p_srv, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> p_srvNormal, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> p_srvMetal, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> p_srvRough);

    // getters
    DirectX::XMFLOAT4 GetColorTint();
    float GetSpecularIntensity();
    //Microsoft::WRL::ComPtr<ID3D11PixelShader> GetPixelShader();
    //Microsoft::WRL::ComPtr<ID3D11VertexShader> GetVertexShader();

    SimplePixelShader* GetPixelShader();
    SimpleVertexShader* GetVertexShader();

    Microsoft::WRL::ComPtr<ID3D11SamplerState> GetSampler();
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetSRV();
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetSRVNormal();
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetSRVMetalness();
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetSRVRoughness();

    // setters
    void SetColorTint(DirectX::XMFLOAT4 cTint);
private:
    DirectX::XMFLOAT4 colorTint;
    //Microsoft::WRL::ComPtr<ID3D11PixelShader> pShader;
    //Microsoft::WRL::ComPtr<ID3D11VertexShader> vShader;
    float specularIntensity;

    SimpleVertexShader* vShader;
    SimplePixelShader* pShader;

    Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvNormal;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvMetal;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvRough;
};

