#include "Material.h"

Material::Material(DirectX::XMFLOAT4 cTint, SimplePixelShader* pixel, SimpleVertexShader* vertex, float sIntensity, Microsoft::WRL::ComPtr<ID3D11SamplerState> p_sampler, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> p_srv, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> p_srvNormal, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> p_srvMetal, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> p_srvRough)
{
    this->colorTint = cTint;
    this->pShader = pixel;
    this->vShader = vertex;
    this->specularIntensity = sIntensity;
    this->sampler = p_sampler;
    this->srv = p_srv;
    this->srvNormal = p_srvNormal;
    this->srvMetal = p_srvMetal;
    this->srvRough = p_srvRough;
}

// getters
DirectX::XMFLOAT4 Material::GetColorTint() { return this->colorTint; }
float Material::GetSpecularIntensity() {return this->specularIntensity; }
SimplePixelShader* Material::GetPixelShader() { return this->pShader; }
SimpleVertexShader* Material::GetVertexShader() { return this->vShader; }
Microsoft::WRL::ComPtr<ID3D11SamplerState> Material::GetSampler() { return this->sampler; }
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Material::GetSRV() { return srv; }
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Material::GetSRVNormal() { return srvNormal; }
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Material::GetSRVMetalness() { return srvMetal; }
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Material::GetSRVRoughness() { return srvRough; }

// setters
void Material::SetColorTint(DirectX::XMFLOAT4 cTint) { colorTint = cTint; }
