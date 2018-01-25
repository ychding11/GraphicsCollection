#ifndef OCEAN_SURFACE_H_
#define OCEAN_SURFACE_H_

#include "DXUT.h"
//#include "DXUTcamera.h"
//#include "PlaneMesh.h"
#include "Camera.h"
#include "D3D11Effect.h"

#include "PerlinNoise.h"
#include <vector>
#include <string>
#include <map>

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
        XMFLOAT4X4 mViewProj;
        XMFLOAT4   vMeshColor;
    };

    struct CBDrawFrustum
    {
        XMFLOAT4X4 mInvViewProj;
        XMFLOAT4X4 mViewProj;
        XMFLOAT4   vMeshColor;
    };

    struct CBWireframe
    {
        XMFLOAT4   vMeshColor;
    };

    struct Position
    {
        float x;
        float y;
        float z;
        float w;
    };

    struct Normal
    {
        float x;
        float y;
        float z;
    };

    struct TexCoord
    {
        float x;
        float y;
    };

    enum Effect
    {
       E_DrawFrustum,
       E_EFFECTS
    };

public:

    int mXSize;
    int mYSize;
    std::map<std::string, int> iparameters;
    std::map<std::string, float> fparameters;

    ID3D11Buffer*			mpIndexBuffer  = nullptr;
    ID3D11Buffer*			mpVertexBuffer = nullptr;
    ID3D11Buffer*			mpNormalBuffer = nullptr;
    ID3D11Buffer*			mpTexCoordBuffer = nullptr;
    ID3D11Buffer*           mpCBChangesEveryFrame = nullptr;
    ID3D11Buffer*           mpCBWireframe = nullptr;
    ID3D11Buffer*           mpCBDrawFrustum = nullptr;
    ID3D11RasterizerState*  mpRSWireframe = nullptr;
    ID3D11RasterizerState*  mpRSSolid = nullptr;

    D3D11Effect mDrawFrustum;
    D3D11Effect mCPUEffect;
    D3D11Effect mWaterEffect;


    XMVECTOR mUpper;
    XMVECTOR mLow;
    XMVECTOR mBase;
    XMVECTOR mNormal;
    XMVECTOR mGridConer[4];
    XMMATRIX mmWorld;
    std::vector<XMVECTOR> mIntersectionPoints;
    std::vector<Position> mSurfaceVertex;
    std::vector<Normal> mSurfaceNormal;
    std::vector<TexCoord> mSurfaceTexCoord;
    std::vector<int> mSurfaceIndex;

    //wireframe color
    XMFLOAT4 mvMeshColor;
    const XMFLOAT4 cvGreen;
    const XMFLOAT4 cvRed;
    PerlinNoise noise;

private:
// for debug        
    Camera mObserveCamera;

public:
    OceanSurface()
        : mXSize(32), mYSize(32)
        , mvMeshColor(0.0, 0.0, 0.0, 1.0)
        , cvGreen(0.0, 1.0, 0.0, 1.0)
        , cvRed(1.0, 0.0, 0.0, 1.0)
        , mObserveCamera(XMFLOAT3(400, 0.0, 0.0), XMFLOAT3(0, 0, 0), XM_PI * 0.25f, 1.78, 0.1f, 10000.0f)
        //, noise(this->mXSize, this->mYSize, 4, 0.99)
        , noise(256, 256, 9, 0.99)
        , mWaterEffect(0x1 | 0x2 | 0x4)
    {
        mmWorld = XMMatrixIdentity();
        mNormal = XMLoadFloat4(&XMFLOAT4(0, 1, 0, 0));
        mBase   = XMLoadFloat4(&XMFLOAT4(0, 1, 0, 0));
        iparameters["prime_index"] = 0;
        iparameters["primitive_topology"] = 2;
        iparameters["wireframe"] = 1;
        iparameters["cputransform"] = 1;
        iparameters["xsize"] = 32;
        iparameters["ysize"] = 32;
        fparameters["max_amplitude"] = 1.0f;

        mUpper = XMPlaneFromPointNormal( XMLoadFloat4(&XMFLOAT4(0, 0, 0, 1.0)) + fparameters["max_amplitude"] * mNormal, mNormal);
        mLow   = XMPlaneFromPointNormal( XMLoadFloat4(&XMFLOAT4(0, 0, 0, 1.0)) - fparameters["max_amplitude"] * mNormal, mNormal);
    }

    void setPrimeIndex(int index)
    {
        iparameters["prime_index"] = index;
        noise.primeIndex = index;
    }

    void setMaxAmplitude(float amplitude)
    {
        fparameters["max_amplitude"] = amplitude;
        mUpper = XMPlaneFromPointNormal( XMLoadFloat4(&XMFLOAT4(0, 0, 0, 1.0)) + fparameters["max_amplitude"] * mNormal, mNormal);
        mLow   = XMPlaneFromPointNormal( XMLoadFloat4(&XMFLOAT4(0, 0, 0, 1.0)) - fparameters["max_amplitude"] * mNormal, mNormal);
        
    }

    ~OceanSurface()
    {
        Destroy();
    }

    void Destroy()
    {
        SAFE_RELEASE( mpIndexBuffer);
        SAFE_RELEASE( mpVertexBuffer);
        SAFE_RELEASE( mpNormalBuffer);
        SAFE_RELEASE( mpTexCoordBuffer);
        SAFE_RELEASE( mpCBChangesEveryFrame);
        SAFE_RELEASE( mpCBWireframe);
        SAFE_RELEASE( mpRSSolid);
        SAFE_RELEASE( mpRSWireframe);
        SAFE_RELEASE( mpCBDrawFrustum);
        mDrawFrustum.Destroy();
        mCPUEffect.Destroy();
        mWaterEffect.Destroy();
    }

    void setSize(int x, int y)
    {
        mXSize = x;
        mYSize = y;
    }

private:

    HRESULT CreateConstBuffer(ID3D11Device* pd3dDevice);
    HRESULT CreateRasterState(ID3D11Device* pd3dDevice);

public:

    HRESULT InitD3D(ID3D11Device* pd3dDevice)
    {
        HRESULT hr = S_OK;
        V_RETURN( CreateConstBuffer( pd3dDevice) );
        V_RETURN( CreateRasterState( pd3dDevice) );
        
        mDrawFrustum.InitEffect(pd3dDevice, L"drawfrustum.fx", "DrawFrustumVS", "DrawFrustumPS");
        mCPUEffect.InitEffect(pd3dDevice, L"cpueffect.fx", "VS", "PS");
        mWaterEffect.InitEffect(pd3dDevice, L"oceanSimulation.fx", "VS", "PS");
        return hr;
    }

public:

    void Render(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, const Camera &renderCamera);
    void ObserveRenderCameraFrustum(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, const Camera &renderCamera);
    
public:
    void SetMeshColor(XMFLOAT4 color)
    { this->mvMeshColor = color; }
    void setWindowAspect(float aspect)
    { mObserveCamera.UpdateAspect(aspect); }

private:

    bool IntersectionTest(const Camera &renderCamera);
    void GetSurfaceRange(const Camera &renderCamera);
    void TessellateSurfaceMesh(const Camera &renderCamera);
    XMVECTOR getWorldGridConer(XMFLOAT2 coner,  const XMMATRIX &invViewprojMat);
    void UpdateParameters(ID3D11DeviceContext* pd3dImmediateContext, const Camera &renderCamera);
    
    HRESULT CreateAndUpdateSurfaceMeshBuffer(ID3D11Device* pd3dDevice);
    HRESULT CreateFrustumBufferCPUMode(ID3D11Device* pd3dDevice,  const Camera &renderCamera);
    HRESULT CreateUpdateFrustumBufferGPUMode(ID3D11Device* pd3dDevice,  const Camera &renderCamera);
};

#endif


