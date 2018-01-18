#ifndef OCEAN_SURFACE_H_
#define OCEAN_SURFACE_H_

#include "DXUT.h"
//#include "DXUTcamera.h"
//#include "PlaneMesh.h"
#include "Camera.h"
#include "PerlinNoise.h"
#include <vector>

#pragma warning( disable : 4100 )

using namespace DirectX;

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------

class OceanSurface
{
public:
    struct CBChangesEveryFrame
    {
        XMFLOAT4X4 mWorld;
        XMFLOAT4X4 mView;
        XMFLOAT4X4 mProj;
        XMFLOAT4   vMeshColor;
    };

    struct SurfaceVertex
    {
        float x;
        float y;
        float z;
        float w;
    };

public:

    int mXSize;
    int mYSize;

    ID3D11VertexShader*     mpVertexShader = nullptr;
    ID3D11PixelShader*      mpPixelShader = nullptr;
    ID3D11InputLayout*      mpVertexLayout = nullptr;
    ID3D11Buffer*			mpIndexBuffer  = nullptr;
    ID3D11Buffer*			mpVertexBuffer = nullptr;
    ID3D11Buffer*           mpCBChangesEveryFrame = nullptr;
    LPCWSTR                 mEffectsFile;

    XMVECTOR mUpper;
    XMVECTOR mLow;
    XMVECTOR mBase;
    XMVECTOR mNormal;
    XMVECTOR mGridConer[4];
    XMMATRIX mmWorld;
    XMFLOAT4 mvMeshColor;
    std::vector<XMVECTOR> mIntersectionPoints;
    std::vector<SurfaceVertex> mSurfaceVertex;
    std::vector<int> mSurfaceIndex;
    PerlinNoise noise;

private:
// for debug        
    Camera mObserveCamera;

public:
    OceanSurface()
        : mEffectsFile(L"oceanSimulation.fx")
        , mXSize(32), mYSize(32)
        , mvMeshColor(0.0, 1.0, 0.0, 1.0)
        , mObserveCamera(XMFLOAT3(400, 0.0, 0.0), XMFLOAT3(0,0,0), XM_PI * 0.25f, 1.78, 0.1f, 10000.0f)
        //, noise(this->mXSize, this->mYSize, 4, 0.99)
        , noise(256, 256, 9, 0.99)
    {
        mmWorld = XMMatrixIdentity();
        mNormal = XMLoadFloat4(&XMFLOAT4(0, 1, 0, 0));
        mBase   = XMLoadFloat4(&XMFLOAT4(0, 1, 0, 0));
        mUpper = XMPlaneFromPointNormal( XMLoadFloat4(&XMFLOAT4(0, 0, 0, 1.0)) + mNormal, mNormal);
        mLow   = XMPlaneFromPointNormal( XMLoadFloat4(&XMFLOAT4(0, 0, 0, 1.0)) - mNormal, mNormal);
    }

    ~OceanSurface()
    {
        Destroy();
    }

    void Destroy()
    {
        SAFE_RELEASE( mpVertexLayout );
        SAFE_RELEASE( mpIndexBuffer);
        SAFE_RELEASE( mpVertexBuffer);
        SAFE_RELEASE( mpCBChangesEveryFrame);
        SAFE_RELEASE( mpVertexShader);
        SAFE_RELEASE( mpPixelShader);
    }

    void setSize(int x, int y)
    {
        mXSize = x;
        mYSize = y;
    }

public:

    HRESULT CreateEffects(ID3D11Device* pd3dDevice, void* pUserContext);
    HRESULT CreateConstBuffer(ID3D11Device* pd3dDevice);

    void SetMeshColor(XMFLOAT4 color)
    {
        this->mvMeshColor = color;
    }
    void setWindowAspect(float aspect)
    {
        mObserveCamera.UpdateAspect(aspect);
    }
private:

    HRESULT CreateAndUpdateSurfaceMeshBuffer(ID3D11Device* pd3dDevice);
    HRESULT CreateFrustumBuffer(ID3D11Device* pd3dDevice,  const Camera &renderCamera);

    bool IntersectionTest(const Camera &renderCamera);
    void GetSurfaceRange(const Camera &renderCamera);
    void TessellateSurfaceMesh(const Camera &renderCamera);
    void UpdateParameters(ID3D11DeviceContext* pd3dImmediateContext, const Camera &renderCamera);
    XMVECTOR getWorldGridConer(XMFLOAT2 coner,  const XMMATRIX &invViewprojMat);
    
public:

    void Render(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, const Camera &renderCamera)
    {
        if (!IntersectionTest(renderCamera)) return;
        GetSurfaceRange(renderCamera);
        noise.primeIndex++;
        TessellateSurfaceMesh(renderCamera);
        CreateAndUpdateSurfaceMeshBuffer(pd3dDevice);
        UpdateParameters(pd3dImmediateContext, renderCamera);

        // Render the mesh
        UINT Strides[1] = { sizeof(SurfaceVertex) };
        UINT Offsets[1] = { 0 };
        ID3D11Buffer* pVB[1] = { mpVertexBuffer };
        pd3dImmediateContext->IASetVertexBuffers( 0, 1, pVB, Strides, Offsets );
        pd3dImmediateContext->IASetIndexBuffer(mpIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
        // Set the Vertex Layout
        pd3dImmediateContext->IASetInputLayout( mpVertexLayout );
        pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        pd3dImmediateContext->VSSetShader(mpVertexShader, nullptr, 0);
        pd3dImmediateContext->VSSetConstantBuffers(0, 1, &mpCBChangesEveryFrame);
        pd3dImmediateContext->PSSetShader(mpPixelShader, nullptr, 0);
        pd3dImmediateContext->PSSetConstantBuffers(0, 1, &mpCBChangesEveryFrame);
        pd3dImmediateContext->DrawIndexed( mSurfaceIndex.size(), 0, 0);
    }

    void ObserveRenderCameraFrustum(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, const Camera &renderCamera)
    {
        CreateFrustumBuffer(pd3dDevice, renderCamera);
        UpdateParameters(pd3dImmediateContext, mObserveCamera);

        // Render the mesh
        UINT Strides[1] = { 16 };
        UINT Offsets[1] = { 0 };
        ID3D11Buffer* pVB[1] = { mpVertexBuffer };
        // Set the Vertex Layout
        pd3dImmediateContext->IASetInputLayout(mpVertexLayout);
        pd3dImmediateContext->IASetVertexBuffers(0, 1, pVB, Strides, Offsets);
        pd3dImmediateContext->IASetIndexBuffer(mpIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
        pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        pd3dImmediateContext->VSSetShader(mpVertexShader, nullptr, 0);
        pd3dImmediateContext->VSSetConstantBuffers(0, 1, &mpCBChangesEveryFrame);
        pd3dImmediateContext->PSSetShader(mpPixelShader, nullptr, 0);
        pd3dImmediateContext->DrawIndexed(24, 0, 0);
    }
};

#endif


