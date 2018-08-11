
#include "BezierSurface.h"
#include "ShaderContainer.h"
#include "WICTextureLoader.h"


#undef V
#define V(x)           { hr = (x); }
#undef  V_RETURN
#define V_RETURN(x)    { hr = (x); if( FAILED(hr) ) { return hr; } }

#if defined(PROFILE) || defined(DEBUG)
inline void SetDebugName(_In_ IDXGIObject* pObj, _In_z_ const CHAR* pstrName)
{
    if (pObj)
        pObj->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen(pstrName), pstrName);
}
inline void SetDebugName(_In_ ID3D11Device* pObj, _In_z_ const CHAR* pstrName)
{
    if (pObj)
        pObj->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen(pstrName), pstrName);
}
inline void SetDebugName(_In_ ID3D11DeviceChild* pObj, _In_z_ const CHAR* pstrName)
{
    if (pObj)
        pObj->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen(pstrName), pstrName);
}
#else
#define SetDebugName( pObj, pstrName )
#endif

static RenderOption renderOption;
RenderOption& RenderOption::getRenderOption()
{
    return renderOption;
}

static Camera modelCamera;

Camera& CameraManager::getCamera()
{
    return modelCamera;
}

BezierSurface& BezierSurfaceManager::getBezierSurface(std::string name )
{
    static BezierSurface* surface = new BezierSurface();
    return *surface;

}

using namespace DirectX;


static XMMATRIX   tempWorld(1, 0,  0, 0,
                    0, 0, -1, 0,
                    0, 1,  0, 0,
                    0, 0,  0, 1);
void BezierSurface::UpdateCBParam(ID3D11DeviceContext* pd3dImmediateContext)
{
    Camera &camera = CameraManager::getCamera();
    const RenderOption & renderOption = RenderOption::getRenderOption();

    // WVP
    XMMATRIX mProj = camera.Proj();
    XMMATRIX mView = camera.View();
    XMMATRIX mViewProjection = camera.ViewProj();

    D3D11_MAPPED_SUBRESOURCE MappedResource;
    pd3dImmediateContext->Map(mpcbFrameParam, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
    auto pData = reinterpret_cast<FrameParam*>(MappedResource.pData);
    XMStoreFloat4x4(&pData->cbWorld, XMMatrixTranspose(tempWorld));
    XMStoreFloat4x4(&pData->cbViewProjection, XMMatrixTranspose(mViewProjection));
    //XMStoreFloat3(&pData->cbCameraPosWorld, camera.GetPosition());
    pData->cbCameraPosWorld = camera.GetPosition();
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

HRESULT BezierSurface::CreateD3D11GraphicsObjects(ID3D11Device*  pd3dDevice)
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
    SetDebugName(mpcbFrameParam, "CB_PER_FRAME_Const_Buffer");

    ////////////////////////////////////////////////////////////////////////
    /// rasterizer state objects
    ////////////////////////////////////////////////////////////////////////
    D3D11_RASTERIZER_DESC RasterDesc;
    ZeroMemory(&RasterDesc, sizeof(D3D11_RASTERIZER_DESC));
    RasterDesc.FillMode = D3D11_FILL_SOLID;
    RasterDesc.CullMode = D3D11_CULL_NONE;
    RasterDesc.DepthClipEnable = TRUE;
    V_RETURN(pd3dDevice->CreateRasterizerState(&RasterDesc, &mpRSSolid));
    SetDebugName(mpRSSolid, "Solid_RS");
    RasterDesc.FillMode = D3D11_FILL_WIREFRAME;
    V_RETURN(pd3dDevice->CreateRasterizerState(&RasterDesc, &mpRSWireframe));
    SetDebugName(mpRSWireframe, "Wireframe_RS");

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
    SetDebugName(mpControlPointVB, "Control Points VB");

    ////////////////////////////////////////////////////////////////////////
    /// Index Buffer objects
    ////////////////////////////////////////////////////////////////////////
    vbDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    vbDesc.ByteWidth   = mMeshData->IBufferSize();
    vbInitData.pSysMem = mMeshData->IBuffer();
    V_RETURN(pd3dDevice->CreateBuffer(&vbDesc, &vbInitData, &mpControlPointIB));
    SetDebugName(mpControlPointIB, "Control Points IB");

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
    V_RETURN(CreateWICTextureFromFile(pd3dDevice, L"heightmap.png",nullptr, &mpHeightMapSRV));

    ////////////////////////////////////////////////////////////////////////
    /// blend state objects
    ////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////
    /// depth stencil state objects
    ////////////////////////////////////////////////////////////////////////
}

void BezierSurface::Render(ID3D11DeviceContext* pd3dImmediateContext)
{
    Shader& shader = ShaderContainer::getShaderContainer()[".\\shader\\TesseQuad.hlsl"];

    UINT Stride = sizeof(ControlPoint);
    UINT Offset = 0;
    pd3dImmediateContext->IASetInputLayout(shader.getInputLayout("ControlPointLayout"));
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
    pd3dImmediateContext->VSSetShader(shader.getVertexShader("PlainVertexShader"), nullptr, 0);
    pd3dImmediateContext->HSSetShader(shader.getHullShader("HullShader"), nullptr, 0);
    pd3dImmediateContext->DSSetShader(shader.getDomainShader("DomainShader"), nullptr, 0);
    pd3dImmediateContext->GSSetShader(nullptr, nullptr, 0);

    pd3dImmediateContext->DSSetSamplers(0, 1, &mpSamplerLinear);
    pd3dImmediateContext->DSSetShaderResources(0, 1, &mpHeightMapSRV);

    // Diag mode
    if (RenderOption::getRenderOption().diagModeOn)
    {
        pd3dImmediateContext->RSSetState(mpRSSolid);
        pd3dImmediateContext->PSSetShader(shader.getPixelShader("DiagPixelShader"), nullptr, 0);
        pd3dImmediateContext->DrawIndexed(mMeshData->IBufferElement(), 0, 0);
    }
    else if (RenderOption::getRenderOption().wireframeOn)
    {
        RenderOption::getRenderOption().wireframeOn = false;
        UpdateCBParam(pd3dImmediateContext);
        pd3dImmediateContext->RSSetState(mpRSSolid);
        pd3dImmediateContext->PSSetShader(shader.getPixelShader("PlainPixelShader"), nullptr, 0);
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
        pd3dImmediateContext->PSSetShader(shader.getPixelShader("PlainPixelShader"), nullptr, 0);
        pd3dImmediateContext->DrawIndexed(mMeshData->IBufferElement(), 0, 0);
    }
}
