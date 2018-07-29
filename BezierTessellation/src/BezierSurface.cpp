
#include "BezierSurface.h"
#include "ShaderManager.h"
#include "utahTeapot.h"


static RenderOption renderOption;
RenderOption& RenderOption::getRenderOption()
{
    return renderOption;
}

static CModelViewerCamera modelCamera;

CModelViewerCamera& CameraManager::getCamera()
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
    CModelViewerCamera &camera = CameraManager::getCamera();
    // WVP
    XMMATRIX mProj = camera.GetProjMatrix();
    XMMATRIX mView = camera.GetViewMatrix();
    XMMATRIX mViewProjection = mView * mProj;

    D3D11_MAPPED_SUBRESOURCE MappedResource;
    pd3dImmediateContext->Map(mpcbFrameParam, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
    auto pData = reinterpret_cast<FrameParam*>(MappedResource.pData);
    XMStoreFloat4x4(&pData->cbWorld, XMMatrixTranspose(tempWorld));
    XMStoreFloat4x4(&pData->cbViewProjection, XMMatrixTranspose(mViewProjection));
    XMStoreFloat3(&pData->cbCameraPosWorld, camera.GetEyePt());
    pData->cbTessellationFactor = 16.0f;
    pd3dImmediateContext->Unmap(mpcbFrameParam, 0);

    // for debug purpose
    XMVECTOR tempEyePos = camera.GetEyePt();
    char buf[256];
    sprintf(buf, "- eye position: %f, %f, %f, %f.\n",
        tempEyePos.m128_f32[0],
        tempEyePos.m128_f32[1],
        tempEyePos.m128_f32[2],
        tempEyePos.m128_f32[3]
    );
    OutputDebugStringA(buf);
}

HRESULT BezierSurface::CreateD3D11GraphicsObjects(ID3D11Device*  pd3dDevice)
{
    HRESULT hr;
    D3D11_BUFFER_DESC Desc;
    Desc.Usage = D3D11_USAGE_DYNAMIC;
    Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Desc.MiscFlags = 0;
    Desc.ByteWidth = sizeof(FrameParam);
    V_RETURN(pd3dDevice->CreateBuffer(&Desc, nullptr, &mpcbFrameParam));
    DXUT_SetDebugName(mpcbFrameParam, "CB_PER_FRAME_Const_Buffer");

    // Create solid and wireframe rasterizer state objects
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

    D3D11_BUFFER_DESC vbDesc;
    ZeroMemory(&vbDesc, sizeof(D3D11_BUFFER_DESC));
    vbDesc.ByteWidth = sizeof(teapotVertices);
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vbInitData;
    ZeroMemory(&vbInitData, sizeof(vbInitData));
    vbInitData.pSysMem = teapotVertices;
    V_RETURN(pd3dDevice->CreateBuffer(&vbDesc, &vbInitData, &mpControlPointVB));
    DXUT_SetDebugName(mpControlPointVB, "Control Points VB");

    // Teapot control points index
    vbDesc.ByteWidth = sizeof(teapotPatches);
    vbDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    vbInitData.pSysMem = teapotPatches;
    V_RETURN(pd3dDevice->CreateBuffer(&vbDesc, &vbInitData, &mpControlPointIB));
    DXUT_SetDebugName(mpControlPointIB, "Control Points IB");

    ShaderManager::getShaderManager().InitD3D11ShaderObjects(pd3dDevice);
}

void BezierSurface::Render(ID3D11DeviceContext* pd3dImmediateContext)
{
    ShaderManager& shdmgr = ShaderManager::getShaderManager();
    UINT Stride = sizeof(ControlPoint);
    UINT Offset = 0;
    pd3dImmediateContext->IASetInputLayout(shdmgr.getInputLayout("ControlPointLayout"));
    pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST);
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
    pd3dImmediateContext->PSSetShader(shdmgr.getPixelShader("WireframePixelShader"), nullptr, 0);

    if (RenderOption::getRenderOption().wireframeOn)
        pd3dImmediateContext->RSSetState(mpRSWireframe);
    else 
        pd3dImmediateContext->RSSetState(mpRSSolid);


    // Draw Pass one
    pd3dImmediateContext->DrawIndexed(kTeapotNumPatches * 16, 0, 0);

    //
    if (RenderOption::getRenderOption().wireframeOn && RenderOption::getRenderOption().wireframeOnShaded)
    {
        pd3dImmediateContext->RSSetState(mpRSSolid);
        pd3dImmediateContext->PSSetShader(shdmgr.getPixelShader("PlainPixelShader"), nullptr, 0);
        pd3dImmediateContext->DrawIndexed(kTeapotNumPatches * 16, 0, 0);
    }
    else
    {

    }
}
