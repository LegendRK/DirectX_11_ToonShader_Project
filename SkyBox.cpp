#include "SkyBox.h"

using namespace DirectX;

SkyBox::SkyBox(Mesh* p_skyMesh, Microsoft::WRL::ComPtr<ID3D11SamplerState> p_skySS, Microsoft::WRL::ComPtr<ID3D11Device> device, std::wstring filePath, SimpleVertexShader* p_skyVS, SimplePixelShader* p_skyPS)
{
    // create the SRV from the dds texture file
    CreateDDSTextureFromFile(device.Get(), filePath.c_str(), nullptr, skySRV.GetAddressOf());

    // set the common constructor values
    SetGeneralParamaters(p_skyMesh, p_skySS, device, p_skyVS, p_skyPS);
}

SkyBox::SkyBox(Mesh* p_skyMesh, Microsoft::WRL::ComPtr<ID3D11SamplerState> p_skySS, Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, SimpleVertexShader* p_skyVS, SimplePixelShader* p_skyPS, std::wstring filePath_right, std::wstring filePath_left, std::wstring filePath_up, std::wstring filePath_down, std::wstring filePath_front, std::wstring filePath_back)
{
    // create a cube map and get the SRV
    skySRV = CreateCubemap(device, context, filePath_right.c_str(), filePath_left.c_str(), filePath_up.c_str(), filePath_down.c_str(), filePath_front.c_str(), filePath_back.c_str());

    // set the common constructor values
    SetGeneralParamaters(p_skyMesh, p_skySS, device, p_skyVS, p_skyPS);
}

void SkyBox::SetGeneralParamaters(Mesh* p_skyMesh, Microsoft::WRL::ComPtr<ID3D11SamplerState> p_skySS, Microsoft::WRL::ComPtr<ID3D11Device> device, SimpleVertexShader* p_skyVS, SimplePixelShader* p_skyPS)
{
    skyMesh = p_skyMesh;
    skySS = p_skySS;

    // generate rasterizer state description and pass it through device to create it
    D3D11_RASTERIZER_DESC rd = {};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_FRONT;
    device->CreateRasterizerState(&rd, skyRS.GetAddressOf());

    // generate depth-stencil description and pass through device to create it.
    D3D11_DEPTH_STENCIL_DESC dsd = {};
    dsd.DepthEnable = true;
    dsd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    device->CreateDepthStencilState(&dsd, skyDS.GetAddressOf());

    skyVS = p_skyVS;
    skyPS = p_skyPS;
}

void SkyBox::Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, Camera* camera)
{
    // Change the render states
    context->RSSetState(skyRS.Get());
    context->OMSetDepthStencilState(skyDS.Get(), 0);

    // Prepare the sky-specific shaders, pass SamplerState and SRV, pass view and projection matrices
    skyVS->SetShader();
    skyPS->SetShader();

    skyPS->SetSamplerState("samplerOptions", skySS.Get());
    skyPS->SetShaderResourceView("skyTexture", skySRV.Get());

    skyVS->SetMatrix4x4("view", camera->GetViewMatrix());
    skyVS->SetMatrix4x4("projection", camera->GetProjectionMatrix());
    skyVS->CopyAllBufferData();

    // Draw the Mesh
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, skyMesh->GetVertexBuffer().GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(skyMesh->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);
    context->DrawIndexed(
        skyMesh->GetIndexCount(),
        0,
        0
    );

    // Reset the render states
    context->RSSetState(nullptr);
    context->OMSetDepthStencilState(nullptr, 0);
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SkyBox::CreateCubemap(
    Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, 
    const wchar_t* right, 
    const wchar_t* left, 
    const wchar_t* up, 
    const wchar_t* down, 
    const wchar_t* front, 
    const wchar_t* back
)
{
    // Load the 6 textures into an array
    // - We need references to the TEXTURES, not the SHADER RESOURCES VIEWS!
    // - Specifically NOT generating mipmaps, as we usually don't need them for the sky!
    // - Order matters here! +x, -X, +y, -Y, +Z, -Z
    ID3D11Texture2D* textures[6] = {};
    CreateWICTextureFromFile(device.Get(), right, (ID3D11Resource**)&textures[0], 0);
    CreateWICTextureFromFile(device.Get(), left, (ID3D11Resource**)&textures[1], 0);
    CreateWICTextureFromFile(device.Get(), up, (ID3D11Resource**)&textures[2], 0);
    CreateWICTextureFromFile(device.Get(), down, (ID3D11Resource**)&textures[3], 0);
    CreateWICTextureFromFile(device.Get(), front, (ID3D11Resource**)&textures[4], 0);
    CreateWICTextureFromFile(device.Get(), back, (ID3D11Resource**)&textures[5], 0);

    // Assume all of the textures are the same color format and resolution,
    // so get the description of the first shader resource view
    D3D11_TEXTURE2D_DESC faceDesc = {};
    textures[0]->GetDesc(&faceDesc);

    // Describe the resource for the cube map, which is simple
    // a "texture 2D array". This is a special GPU resource format
    // NOT just a C++ array of textures!!!
    D3D11_TEXTURE2D_DESC cubeDesc = {};
    cubeDesc.ArraySize = 6;                                 // cube map!
    cubeDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;        // We'll be using as a texture in a shader
    cubeDesc.CPUAccessFlags = 0;                            // No read back
    cubeDesc.Format = faceDesc.Format;                      // Match the loaded texture's color format
    cubeDesc.Width = faceDesc.Width;                        // Match the size
    cubeDesc.Height = faceDesc.Height;                      // Match the size
    cubeDesc.MipLevels = 1;                                 // Only need 1
    cubeDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;   // A CUBE MAP, not 6 seperate textures
    cubeDesc.Usage = D3D11_USAGE_DEFAULT;                   // Standard usage
    cubeDesc.SampleDesc.Count = 1;
    cubeDesc.SampleDesc.Quality = 0;

    // create the actual texture resource
    ID3D11Texture2D* cubeMapTexture = 0;
    device->CreateTexture2D(&cubeDesc, 0, &cubeMapTexture);
    assert(cubeMapTexture != 0);    // remove warnings of cubeMaxTexture being 0
    
    // loop through the individual face textures and copy them
    // one at a time, to the cube map texture
    for (int i = 0; i < 6; i++)
    {
        // to remove warning about textures[i] being 0
        assert(textures[i] != 0);

        // Calculate the subresource position to copy into
        unsigned int subresource = D3D11CalcSubresource(
            0,      // Which mip (zero, since there is only one
            i,      // Which array element
            1       // How many mip levels in the texture
        );

        // copy from one resource (texture) to another
        context->CopySubresourceRegion(
            cubeMapTexture,     // destination resource
            subresource,        // destination subresource index (one of the array elements)
            0, 0, 0,            // XYZ location of copy
            textures[i],        // Source resource
            0,                  // Source subresource index (assuming only one)
            0                   // Source subresource "box" of data to copy (0 means the whole thing)
        );
    }
    
    // At this point, all of the faces have been copied into the
    // cube map texture, so we can describe a shader resource view for it
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = cubeDesc.Format;                           // same format as texture
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;    // Treat this as a cube
    srvDesc.TextureCube.MipLevels = 1;                          // Only need access to 1 mip
    srvDesc.TextureCube.MostDetailedMip = 0;                    // Index of the first mip we want to see

    // Make the SRV
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cubeSRV;
    if (cubeMapTexture != 0)
        device->CreateShaderResourceView(cubeMapTexture, &srvDesc, cubeSRV.GetAddressOf());

    // clean up stuff we don't need anymore
    cubeMapTexture->Release();
    for (int i = 0; i < 6; i++)
    {
        textures[i]->Release();
    }
    
    return cubeSRV;


}
