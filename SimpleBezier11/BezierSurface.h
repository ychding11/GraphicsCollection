#include <vector>
#include <d3d11.h>
#include <DirectXMath.h>
#include "DXUT.h"
#include "SDKmisc.h"

struct CB_PER_FRAME_CONSTANTS
{
    DirectX::XMFLOAT4X4  mWorld;
    DirectX::XMFLOAT4X4  mViewProjection;
    DirectX::XMFLOAT3    vCameraPosWorld;
    float                fTessellationFactor;
};

class BezierSurface
{
public:
    struct ControlPoint
    {
        float cp[3];
    };
    enum RasterState
    {
       WireFrame,
       Solid,
    };

    enum PartitionMode
    {
        PARTITION_INTEGER,
        PARTITION_FRACTIONAL_EVEN,
        PARTITION_FRACTIONAL_ODD
    };

private:
    std::vector<ControlPoint> mControlPoints;
    std::vector<int> mIndex;
    RasterState mRasterState;
    PartitionMode mPartitionMode;

    ID3D11Buffer*             mpcbPerFrame = nullptr;
    ID3D11Buffer*             mpcbMaterial = nullptr;
    ID3D11Buffer*             mpControlPointVB = nullptr;
    ID3D11Buffer*             mpControlPointIB = nullptr;
    ID3D11RasterizerState*    mpRasterizerStateSolid = nullptr;
    ID3D11RasterizerState*    mpRasterizerStateWireframe = nullptr;

public:
    BezierSurface()
        : mRasterState(WireFrame)
        , mPartitionMode(PARTITION_INTEGER)
    {

    }

    ~BezierSurface()
    {

    }

    void DestroyD3D11Objects()
    {
        SAFE_RELEASE( mpControlPointIB );
        SAFE_RELEASE( mpControlPointVB );
        SAFE_RELEASE( mpRasterizerStateWireframe );
        SAFE_RELEASE( mpRasterizerStateSolid );
        SAFE_RELEASE( mpcbPerFrame );
    }

    HRESULT CreateD3D11Objects(ID3D11Device* pd3dDevice)
    {
        HRESULT hr;
        // Create constant buffers
        D3D11_BUFFER_DESC Desc;
        Desc.Usage = D3D11_USAGE_DYNAMIC;
        Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        Desc.MiscFlags = 0;

        Desc.ByteWidth = sizeof(CB_PER_FRAME_CONSTANTS);
        V_RETURN(pd3dDevice->CreateBuffer(&Desc, nullptr, &mpcbPerFrame));
        DXUT_SetDebugName(mpcbPerFrame, "CB_PER_FRAME_CONSTANTS");


        // Create solid and wireframe rasterizer state objects
        D3D11_RASTERIZER_DESC RasterDesc;
        ZeroMemory(&RasterDesc, sizeof(D3D11_RASTERIZER_DESC));
        RasterDesc.FillMode = D3D11_FILL_SOLID;
        RasterDesc.CullMode = D3D11_CULL_NONE;
        RasterDesc.DepthClipEnable = TRUE;
        V_RETURN(pd3dDevice->CreateRasterizerState(&RasterDesc, &mpRasterizerStateSolid));
        DXUT_SetDebugName(mpRasterizerStateSolid, "Solid");
        RasterDesc.FillMode = D3D11_FILL_WIREFRAME;
        V_RETURN(pd3dDevice->CreateRasterizerState(&RasterDesc, &mpRasterizerStateWireframe));
        DXUT_SetDebugName(mpRasterizerStateWireframe, "Wireframe");

        D3D11_BUFFER_DESC vbDesc;
        ZeroMemory(&vbDesc, sizeof(D3D11_BUFFER_DESC));
        vbDesc.ByteWidth = sizeof(ControlPoint) * mControlPoints.size();
        vbDesc.Usage = D3D11_USAGE_DEFAULT;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vbInitData;
        ZeroMemory(&vbInitData, sizeof(vbInitData));
        vbInitData.pSysMem = &mControlPoints[0];
        V_RETURN(pd3dDevice->CreateBuffer(&vbDesc, &vbInitData, &mpControlPointVB));
        DXUT_SetDebugName(mpControlPointVB, "Control Points");

        // Teapot control points index
        vbDesc.ByteWidth = sizeof(mIndex);
        vbDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        vbInitData.pSysMem = &mIndex[0];
        V_RETURN(pd3dDevice->CreateBuffer(&vbDesc, &vbInitData, &mpControlPointIB));
        DXUT_SetDebugName(mpControlPointIB, "Control Points Index");
    }

    void UpdateParameters();
    void SetRasterState(RasterState rs)
    {
        mRasterState = rs;
    }

    void Render(ID3D11DeviceContext* pd3dImmediateContext)
    {
        // Bind all of the CBs
        pd3dImmediateContext->VSSetConstantBuffers(0, 1, &mpcbPerFrame);
        pd3dImmediateContext->HSSetConstantBuffers(0, 1, &mpcbPerFrame);
        pd3dImmediateContext->DSSetConstantBuffers(0, 1, &mpcbPerFrame);
        pd3dImmediateContext->PSSetConstantBuffers(0, 1, &mpcbPerFrame);

        // Set the shaders
        pd3dImmediateContext->VSSetShader(mpVertexShader, nullptr, 0);

        if (mPartitionMode == PARTITION_INTEGER)
            pd3dImmediateContext->HSSetShader(mpHullShaderInteger, nullptr, 0);
        else if (mPartitionMode == PARTITION_FRACTIONAL_EVEN)
            pd3dImmediateContext->HSSetShader(mpHullShaderFracEven, nullptr, 0);
        else if (mPartitionMode == PARTITION_FRACTIONAL_ODD)
            pd3dImmediateContext->HSSetShader(mpHullShaderFracOdd, nullptr, 0);
        else
        { }

        pd3dImmediateContext->DSSetShader(mpDomainShader, nullptr, 0);
        pd3dImmediateContext->GSSetShader(nullptr, nullptr, 0);
        pd3dImmediateContext->PSSetShaderResources(0, 1, &mpTextureRV);
        pd3dImmediateContext->PSSetSamplers(0, 1, &mpSamplerLinear);
        pd3dImmediateContext->RSSetState(mpRasterizerStateSolid);
        pd3dImmediateContext->PSSetShader(mpPixelShader, nullptr, 0);
        if (mRasterState == WireFrame)
        {
            pd3dImmediateContext->RSSetState(mpRasterizerStateWireframe);
            pd3dImmediateContext->PSSetShader(mpSolidColorPS, nullptr, 0);
        }

        // Set the input assembler
        UINT Stride = sizeof(ControlPoint);
        UINT Offset = 0;
        pd3dImmediateContext->IASetInputLayout(mpPatchLayout);
        pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST);
        pd3dImmediateContext->IASetVertexBuffers(0, 1, &mpControlPointVB, &Stride, &Offset);
        pd3dImmediateContext->IASetIndexBuffer(mpControlPointIB, DXGI_FORMAT_R32_UINT, 0);
        pd3dImmediateContext->DrawIndexed(kTeapotNumPatches * 16, 0, 0);
    }
};