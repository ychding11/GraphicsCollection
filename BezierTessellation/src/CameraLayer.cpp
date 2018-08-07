
#include "BezierSurface.h"
#include "CameraLayer.h"
#include "ShaderContainer.h"

using namespace DirectX;

static XMMATRIX   tempWorld(1, 0,  0, 0,
                    0, 0, -1, 0,
                    0, 1,  0, 0,
                    0, 0,  0, 1);
void CameraLayer::UpdateCBParam(ID3D11DeviceContext* pd3dImmediateContext)
{
    Camera &camera = CameraManager::getCamera();
    const RenderOption & renderOption = RenderOption::getRenderOption();

    // WVP
    XMMATRIX mProj = camera.Proj();
    XMMATRIX mView = camera.View();
    XMMATRIX mViewProjection = mView * mProj;

    D3D11_MAPPED_SUBRESOURCE MappedResource;
    pd3dImmediateContext->Map(mpcbFrameParam, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
    auto pData = reinterpret_cast<FrameParam*>(MappedResource.pData);
    XMStoreFloat4x4(&pData->cbWorld, XMMatrixTranspose(tempWorld));
    XMStoreFloat4x4(&pData->cbViewProjection, XMMatrixTranspose(mViewProjection));
    //XMStoreFloat3(&pData->cbCameraPosWorld, camera.GetEyePt());
    pData->cbCameraPosWorld = camera.GetPosition();
    XMStoreFloat3(&pData->cbCameraUp, {0.f, 1.f, 0.f} );
    XMStoreFloat3(&pData->cbCameraRight, {1.f, 0.f, 0.f});
    XMStoreFloat3(&pData->cbCameraForward, {0.f, 0.f, 1.f});
    pData->cbWireframeOn = renderOption.wireframeOn;
    pData->cbTessellationFactor = renderOption.tessellateFactor;
    pData->cbHeightMapOn = renderOption.heightMapOn;
    pData->cbDiagType = renderOption.diagType;
    pData->cbTexelCellU = 0.002f;
    pData->cbTexelCellU = 0.002f;
    pData->cbWorldCell = 0.002f;
    pd3dImmediateContext->Unmap(mpcbFrameParam, 0);

#if 0
    // for debug purpose
    XMVECTOR tempEyePos = camera.GetEyePt();
    char buf[256];
    sprintf(buf, "- eye position: %f, %f, %f, %f.\n",
        tempEyePos.m128_f32[0],
        tempEyePos.m128_f32[1],
        tempEyePos.m128_f32[2],
        tempEyePos.m128_f32[3]
    );
   // OutputDebugStringA(buf);
#endif
}

HRESULT CameraLayer::CreateD3D11GraphicsObjects(ID3D11Device*  pd3dDevice)
{
    HRESULT hr;

    ////////////////////////////////////////////////////////////////////////
    /// Const Buffer
    ////////////////////////////////////////////////////////////////////////
    D3D11_BUFFER_DESC Desc;
    Desc.Usage = D3D11_USAGE_DYNAMIC;
    Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Desc.MiscFlags = 0;
    Desc.ByteWidth = ( sizeof(FrameParam) + 15 ) & ~0xf;
    V_RETURN(pd3dDevice->CreateBuffer(&Desc, nullptr, &mpcbFrameParam));
    DXUT_SetDebugName(mpcbFrameParam, "CB_PER_FRAME_Const_Buffer");

    ////////////////////////////////////////////////////////////////////////
    /// rasterizer state objects
    ////////////////////////////////////////////////////////////////////////
    D3D11_RASTERIZER_DESC RasterDesc;
    ZeroMemory(&RasterDesc, sizeof(D3D11_RASTERIZER_DESC));
    RasterDesc.FillMode = D3D11_FILL_SOLID;
    RasterDesc.CullMode = D3D11_CULL_NONE;
    RasterDesc.DepthClipEnable = TRUE;
    V_RETURN(pd3dDevice->CreateRasterizerState(&RasterDesc, &mpRSSolid));
    DXUT_SetDebugName(mpRSSolid, "Solid_RS");
    RasterDesc.FillMode = D3D11_FILL_WIREFRAME;
    V_RETURN(pd3dDevice->CreateRasterizerState(&RasterDesc, &mpRSWireframe));
    DXUT_SetDebugName(mpRSWireframe, "Wireframe_RS");

    ////////////////////////////////////////////////////////////////////////
    /// Vertex Buffer objects
    ////////////////////////////////////////////////////////////////////////
    D3D11_BUFFER_DESC vbDesc;
    ZeroMemory(&vbDesc, sizeof(D3D11_BUFFER_DESC));
    vbDesc.ByteWidth = mMeshData->VBufferSize();
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vbInitData;
    ZeroMemory(&vbInitData, sizeof(vbInitData));
    vbInitData.pSysMem = mMeshData->VBuffer();
    V_RETURN(pd3dDevice->CreateBuffer(&vbDesc, &vbInitData, &mpControlPointVB));
    DXUT_SetDebugName(mpControlPointVB, "Control Points VB");

    ////////////////////////////////////////////////////////////////////////
    /// Index Buffer objects
    ////////////////////////////////////////////////////////////////////////
    vbDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    vbDesc.ByteWidth   = mMeshData->IBufferSize();
    vbInitData.pSysMem = mMeshData->IBuffer();
    V_RETURN(pd3dDevice->CreateBuffer(&vbDesc, &vbInitData, &mpControlPointIB));
    DXUT_SetDebugName(mpControlPointIB, "Control Points IB");

    ////////////////////////////////////////////////////////////////////////
    /// sample state objects
    ////////////////////////////////////////////////////////////////////////
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    V_RETURN(pd3dDevice->CreateSamplerState(&sampDesc, &mpSamplerLinear));
    V_RETURN(DXUTCreateShaderResourceViewFromFile(pd3dDevice, L".\\texture\\heightmap.png", &mpHeightMapSRV));

    ////////////////////////////////////////////////////////////////////////
    /// blend state objects
    ////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////
    /// depth stencil state objects
    ////////////////////////////////////////////////////////////////////////
}

void CameraLayer::Render(ID3D11DeviceContext* pd3dImmediateContext)
{
    ShaderContainer &shaders = ShaderContainer::getShaderContainer();
    Shader& shader = shaders[".\\shader\\drawCameraVector.hlsl"];

    UINT Stride = sizeof(float) * 3;
    UINT Offset = 0;
    pd3dImmediateContext->IASetInputLayout(shader.getInputLayout("ControlPointLayout"));
    pd3dImmediateContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
    pd3dImmediateContext->IASetVertexBuffers(0, 1, &mpControlPointVB, &Stride, &Offset);
    pd3dImmediateContext->IASetIndexBuffer(mpControlPointIB, DXGI_FORMAT_R32_UINT, 0);

    // Bind all of the CBs
    this->UpdateCBParam(pd3dImmediateContext);
    pd3dImmediateContext->VSSetConstantBuffers(0, 1, &mpcbFrameParam);
    pd3dImmediateContext->HSSetConstantBuffers(0, 1, &mpcbFrameParam);
    pd3dImmediateContext->DSSetConstantBuffers(0, 1, &mpcbFrameParam);
    pd3dImmediateContext->PSSetConstantBuffers(0, 1, &mpcbFrameParam);

    // Set the shaders
    pd3dImmediateContext->VSSetShader(shader.getVertexShader("PlainVertexShader"), nullptr, 0);
    pd3dImmediateContext->HSSetShader(nullptr, nullptr, 0);
    pd3dImmediateContext->DSSetShader(nullptr, nullptr, 0);
    pd3dImmediateContext->GSSetShader(shader.getGeometryShader("GeometryShader"), nullptr, 0);

    pd3dImmediateContext->DSSetSamplers(0, 1, &mpSamplerLinear);
    pd3dImmediateContext->DSSetShaderResources(0, 1, &mpHeightMapSRV);

    pd3dImmediateContext->RSSetState(mpRSSolid);
    pd3dImmediateContext->PSSetShader(shader.getPixelShader("WireframePixelShader"), nullptr, 0);
    pd3dImmediateContext->DrawIndexed(mMeshData->IBufferElement(), 0, 0);

}

static CameraLayer cameraLayer;
CameraLayer& CameraLayer::getCameraLayer()
{
    return cameraLayer;
}
