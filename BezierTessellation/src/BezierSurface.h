#ifndef  BEZIER_SURFACE_H
#define  BEZIER_SURFACE_H

#include <vector>
#include <d3d11.h>
#include <DirectXMath.h>
#include "DXUT.h"
#include "SDKmisc.h"
#include "DXUTcamera.h"
#include "ShaderManager.h"
#include "IDataSource.h"


#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p) = nullptr; } }
#endif

class CameraManager
{
public:
    static CModelViewerCamera& getCamera();
};

struct FrameParam
{
    DirectX::XMFLOAT4X4  cbWorld;
    DirectX::XMFLOAT4X4  cbViewProjection;
    DirectX::XMFLOAT3    cbCameraPosWorld;
    float                cbTessellationFactor;
    bool                 cbWireframeOn;
};

struct RenderOption
{
    bool wireframeOn;
    bool wireframeOnShaded;
    int  tessellateFactor;

    RenderOption::RenderOption()
        : wireframeOn(false)
        , wireframeOnShaded(true)
        , tessellateFactor(10)
    { }

    static RenderOption& getRenderOption();

};

class BezierSurface
{
public:

    struct ControlPoint
    {
        float controlPoint[3];
    };

    enum PartitionMode
    {
        PARTITION_INTEGER,
        PARTITION_FRACTIONAL_EVEN,
        PARTITION_FRACTIONAL_ODD
    };

private:
    IDataSource*  mMeshData;
    PartitionMode mPartitionMode;

    ID3D11Buffer*             mpcbFrameParam = nullptr;
    ID3D11Buffer*             mpcbMaterial = nullptr;
    ID3D11Buffer*             mpControlPointVB = nullptr;
    ID3D11Buffer*             mpControlPointIB = nullptr;
    ID3D11RasterizerState*    mpRSSolid = nullptr;
    ID3D11RasterizerState*    mpRSWireframe = nullptr;

public:
    BezierSurface(void)
        : mPartitionMode(PARTITION_INTEGER)
        , mMeshData(nullptr)
    { }

    ~BezierSurface()
    {
        delete mMeshData; 
    }

    void SetupMeshData(IDataSource *data)
    {
        assert(data);
        mMeshData = data;
    }

    void Initialize(ID3D11Device*  d3dDevice, ID3D11DeviceContext* context)
    {
        CreateD3D11GraphicsObjects(d3dDevice);
    }

    void Render(ID3D11DeviceContext* pd3dImmediateContext);

    void DestroyD3D11Objects()
    {
        SAFE_RELEASE( mpControlPointIB );
        SAFE_RELEASE( mpControlPointVB );
        SAFE_RELEASE( mpRSWireframe );
        SAFE_RELEASE( mpRSSolid );
        SAFE_RELEASE( mpcbFrameParam );
        ShaderManager::getShaderManager().Destroy();
    }


private:
    HRESULT CreateD3D11GraphicsObjects(ID3D11Device*  d3dDevice);

    void UpdateCBParam(ID3D11DeviceContext* pd3dImmediateContex);

};

class BezierSurfaceManager
{
public:
    static BezierSurface& getBezierSurface(std::string name = "");

};

#endif  