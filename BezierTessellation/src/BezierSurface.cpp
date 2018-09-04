
#include "BezierSurface.h"
#include "ShaderManager.h"


#define D3D11_CALL_CHECK(x)                           \
do{                                                   \
    LRESULT ret = x;                                  \
    if((ret) != S_OK)                                 \
    {                                                 \
        char buf[512];                                \
        sprintf_s(buf, 512, "- Error @%s:%d\t  %s %d\t \n",__FILE__,__LINE__, #x, (ret) );  \
        OutputDebugStringA(buf);                      \
    }                                                 \
} while(0)


static RenderOption renderOption;
RenderOption& RenderOption::getRenderOption()
{
    return renderOption;
}

BezierSurface& BezierSurfaceManager::getBezierSurface(std::string name )
{
    static BezierSurface* surface = new BezierSurface();
    return *surface;

}

using namespace DirectX;


static XMMATRIX   tempWorld(1, 0,  0, 0,
                            0, 1,  0, 0,
                            0, 0,  1, 0,
                            0, 0,  0, 1);
void BezierSurface::UpdateCBParam(ID3D11DeviceContext* pd3dImmediateContext)
{
    const RenderOption & renderOption = RenderOption::getRenderOption();

    XMVECTOR eyePos = { 0.0f,  1.f, -1.0f };
    XMVECTOR atPos  = { 0.0f, 0.0f, 0.0f };
    XMVECTOR up     = { 0.0f, 1.0f, 0.0f };
    float length = DirectX::XMVector3Length( XMVECTOR{ 0.0f,  0.f, 1.0f } - atPos).m128_f32[0];
    static float theta = 0.f;
    theta += 0.001 * XM_2PI;
    float x = length * cosf(theta);
    float z = length * sinf(theta);
          
    if (false == renderOption.fixedCamera)
        eyePos = { x,  1.f, z };

    XMMATRIX mView = DirectX::XMMatrixLookAtLH(eyePos, atPos, up);
    XMMATRIX mProj = DirectX::XMMatrixPerspectiveFovLH(XM_PI / 2.f, 1.f, 0.1f, 600.0f);
    XMMATRIX mViewProjection = mView * mProj;


    D3D11_MAPPED_SUBRESOURCE MappedResource;
    pd3dImmediateContext->Map(mpcbFrameParam, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
    auto pData = reinterpret_cast<FrameParam*>(MappedResource.pData);
    DirectX::XMStoreFloat4x4(&pData->cbWorld, DirectX::XMMatrixTranspose(tempWorld));
    DirectX::XMStoreFloat4x4(&pData->cbViewProjection, DirectX::XMMatrixTranspose(mViewProjection));
    DirectX::XMStoreFloat3(&pData->cbCameraPosWorld, eyePos);
    pData->cbWireframeOn = renderOption.wireframeOn;
    pData->cbTessellationFactor = renderOption.tessellateFactor;
    pData->cbHeightMapOn = renderOption.heightMapOn;
    pData->cbDiagType = renderOption.diagType;
    pData->cbTexelCellU = 0.002f;
    pData->cbTexelCellU = 0.002f;
    pData->cbWorldCell = 0.002f;
    pd3dImmediateContext->Unmap(mpcbFrameParam, 0);

}

HRESULT BezierSurface::CreateD3D11GraphicsObjects(ID3D11Device*  pd3dDevice)
{
    HRESULT hr;
    ShaderManager::getShaderManager().InitD3D11ShaderObjects(pd3dDevice);

    ////////////////////////////////////////////////////////////////////////
    /// Const Buffer
    ////////////////////////////////////////////////////////////////////////
    D3D11_BUFFER_DESC Desc;
    Desc.Usage = D3D11_USAGE_DYNAMIC;
    Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Desc.MiscFlags = 0;
    Desc.ByteWidth = ( sizeof(FrameParam) + 15 ) & ~0xf;
    D3D11_CALL_CHECK(pd3dDevice->CreateBuffer(&Desc, nullptr, &mpcbFrameParam));

    ////////////////////////////////////////////////////////////////////////
    /// rasterizer state objects
    ////////////////////////////////////////////////////////////////////////
    D3D11_RASTERIZER_DESC RasterDesc;
    ZeroMemory(&RasterDesc, sizeof(D3D11_RASTERIZER_DESC));
    RasterDesc.FillMode = D3D11_FILL_SOLID;
    RasterDesc.CullMode = D3D11_CULL_NONE;
    RasterDesc.DepthClipEnable = TRUE;
    D3D11_CALL_CHECK(pd3dDevice->CreateRasterizerState(&RasterDesc, &mpRSSolid));
    RasterDesc.FillMode = D3D11_FILL_WIREFRAME;
    D3D11_CALL_CHECK(pd3dDevice->CreateRasterizerState(&RasterDesc, &mpRSWireframe));

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
    D3D11_CALL_CHECK(pd3dDevice->CreateBuffer(&vbDesc, &vbInitData, &mpControlPointVB));

    ////////////////////////////////////////////////////////////////////////
    /// Index Buffer objects
    ////////////////////////////////////////////////////////////////////////
    vbDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    vbDesc.ByteWidth   = mMeshData->IBufferSize();
    vbInitData.pSysMem = mMeshData->IBuffer();
    D3D11_CALL_CHECK(pd3dDevice->CreateBuffer(&vbDesc, &vbInitData, &mpControlPointIB));

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
    D3D11_CALL_CHECK(pd3dDevice->CreateSamplerState(&sampDesc, &mpSamplerLinear));
    D3D11_CALL_CHECK(DXUTCreateShaderResourceViewFromFile(pd3dDevice, L"heightmap.png", &mpHeightMapSRV));

    ////////////////////////////////////////////////////////////////////////
    /// blend state objects
    ////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////
    /// depth stencil state objects
    ////////////////////////////////////////////////////////////////////////
    return S_OK;
}

void BezierSurface::Render(ID3D11DeviceContext* pd3dImmediateContext)
{
    ShaderManager& shdmgr = ShaderManager::getShaderManager();
    UINT Stride = sizeof(ControlPoint);
    UINT Offset = 0;
    pd3dImmediateContext->IASetInputLayout(shdmgr.getInputLayout("ControlPointLayout"));
    //pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST);
    pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);
    pd3dImmediateContext->IASetVertexBuffers(0, 1, &mpControlPointVB, &Stride, &Offset);
    pd3dImmediateContext->IASetIndexBuffer(mpControlPointIB, DXGI_FORMAT_R32_UINT, 0);

    // Bind all of the CBs
    this->UpdateCBParam(pd3dImmediateContext);
    pd3dImmediateContext->VSSetConstantBuffers(0, 1, &mpcbFrameParam);
    pd3dImmediateContext->HSSetConstantBuffers(0, 1, &mpcbFrameParam);
    pd3dImmediateContext->DSSetConstantBuffers(0, 1, &mpcbFrameParam);
    pd3dImmediateContext->PSSetConstantBuffers(0, 1, &mpcbFrameParam);

    // Set the shaders
    pd3dImmediateContext->VSSetShader(shdmgr.getVertexShader("PlainVertexShader"), nullptr, 0);
    pd3dImmediateContext->HSSetShader(shdmgr.getHullShader("HullShader"), nullptr, 0);
    pd3dImmediateContext->DSSetShader(shdmgr.getDomainShader("DomainShader"), nullptr, 0);
    pd3dImmediateContext->GSSetShader(nullptr, nullptr, 0);

    pd3dImmediateContext->DSSetSamplers(0, 1, &mpSamplerLinear);
    pd3dImmediateContext->DSSetShaderResources(0, 1, &mpHeightMapSRV);

    // Diag mode
    if (RenderOption::getRenderOption().diagModeOn)
    {
        pd3dImmediateContext->RSSetState(mpRSSolid);
        pd3dImmediateContext->PSSetShader(shdmgr.getPixelShader("PlainPixelShader"), nullptr, 0);
        pd3dImmediateContext->DrawIndexed(mMeshData->IBufferElement(), 0, 0);
    }
    else if (RenderOption::getRenderOption().wireframeOn)
    {
        RenderOption::getRenderOption().wireframeOn = false;
        UpdateCBParam(pd3dImmediateContext);
        pd3dImmediateContext->RSSetState(mpRSSolid);
        pd3dImmediateContext->PSSetShader(shdmgr.getPixelShader("WireframePixelShader"), nullptr, 0);
        pd3dImmediateContext->DrawIndexed(mMeshData->IBufferElement(), 0, 0);

        RenderOption::getRenderOption().wireframeOn = true;
        UpdateCBParam(pd3dImmediateContext);
        pd3dImmediateContext->RSSetState(mpRSWireframe);
        pd3dImmediateContext->DrawIndexed(mMeshData->IBufferElement(), 0, 0);
        RenderOption::getRenderOption().wireframeOn = true;

    }
    else
    {
        pd3dImmediateContext->RSSetState(mpRSSolid);
        pd3dImmediateContext->PSSetShader(shdmgr.getPixelShader("WireframePixelShader"), nullptr, 0);
        pd3dImmediateContext->DrawIndexed(mMeshData->IBufferElement(), 0, 0);
    }
}
