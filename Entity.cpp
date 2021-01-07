#include "Entity.h"

using namespace DirectX;

Entity::Entity(Mesh* p_Mesh, Material* p_Mat)
{
    mesh = p_Mesh;
    mat = p_Mat;
}

Mesh* Entity::GetMesh() { return this->mesh; }
Material* Entity::GetMaterial() { return this->mat; }
Transform* Entity::GetTransform() { return &transform; }

void Entity::Draw(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, Camera* camera)
{
    // Set the vertex and pixel shaders to use for the next Draw() command
    //  - These don't technically need to be set every frame
    //  - Once you start applying different shaders to different objects,
    //    you'll need to swap the current shaders before each draw
    // context->VSSetShader(mat->GetVertexShader().Get(), 0, 0);
    // context->PSSetShader(mat->GetPixelShader().Get(), 0, 0);
    mat->GetVertexShader()->SetShader();
    mat->GetPixelShader()->SetShader();

    mat->GetPixelShader()->SetSamplerState("SamplerOptions", mat->GetSampler().Get());
    mat->GetPixelShader()->SetShaderResourceView("Albedo", mat->GetSRV().Get());
    if (mat->GetSRVNormal())
        mat->GetPixelShader()->SetShaderResourceView("NormalMap", mat->GetSRVNormal().Get());
    mat->GetPixelShader()->SetShaderResourceView("RoughnessMap", mat->GetSRVRoughness().Get());
    mat->GetPixelShader()->SetShaderResourceView("MetalnessMap", mat->GetSRVMetalness().Get());

    // Set the Constant Buffer resource
    //context->VSSetConstantBuffers(
    //    0,		// which slot/register to bind the buffer to
    //    1,		// how many are we activating? Can do multiple at once
    //    vsConstantBuffer.GetAddressOf()	// Array of buffers or the address of one
    //);

    // collect the data for the current entity in a c++ struct
    /*VertexShaderExternalData vsData;
    vsData.colorTint = mat->GetColorTint();
    vsData.worldMatrix = transform.GetWorldMatrix();
    vsData.viewMatrix = camera->GetViewMatrix();
    vsData.projectionMatrix = camera->GetProjectionMatrix();*/

    SimpleVertexShader* vs = mat->GetVertexShader();
    vs->SetFloat4("colorTint", mat->GetColorTint());
    vs->SetMatrix4x4("world", transform.GetWorldMatrix());
    vs->SetMatrix4x4("view", camera->GetViewMatrix());
    vs->SetMatrix4x4("projection", camera->GetProjectionMatrix());
    vs->SetMatrix4x4("invTransposeWorld", transform.GetInverseTransposeWorldMatrix());

    // map / memcpy / unmap the constant buffer resource
    /*D3D11_MAPPED_SUBRESOURCE mappedBuffer = {};
    context->Map(vsConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedBuffer);
    memcpy(mappedBuffer.pData, &vsData, sizeof(vsData));
    context->Unmap(vsConstantBuffer.Get(), 0);*/
    vs->CopyAllBufferData();

    // Set the Vertex and Index Buffer
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, mesh->GetVertexBuffer().GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(mesh->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);

    // tell D3D to render using the currently bound resources
    // Finally do the actual drawing
        //  - Do this ONCE PER OBJECT you intend to draw
        //  - This will use all of the currently set DirectX "stuff" (shaders, buffers, etc)
        //  - DrawIndexed() uses the currently set INDEX BUFFER to look up corresponding
        //     vertices in the currently set VERTEX BUFFER
    context->DrawIndexed(
        mesh->GetIndexCount(),     // The number of indices to use (we could draw a subset if we wanted)
        0,     // Offset to the first index we want to use
        0);    // Offset to add to each index when looking up vertices

}