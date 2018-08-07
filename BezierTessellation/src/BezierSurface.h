#ifndef  BEZIER_SURFACE_H
#define  BEZIER_SURFACE_H

#include <vector>
#include <d3d11.h>
#include <DirectXMath.h>
#include "Camera.h"
#include "ShaderManager.h"
#include "IDataSource.h"


#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p) = nullptr; } }
#endif

class CameraManager
{
public:
    static Camera& getCamera();
};

enum DiagType
{
    eDiagNormal       = 0,
    eDiagTangent      = 1,
    eDiagBiTangent    = 2,
    eDiagPosition     = 3,
    eDiagNum
};
struct FrameParam
{
    DirectX::XMFLOAT4X4  cbWorld;
    DirectX::XMFLOAT4X4  cbViewProjection;
    DirectX::XMFLOAT3    cbCameraPosWorld;
    DirectX::XMFLOAT3    cbCameraUp;
    DirectX::XMFLOAT3    cbCameraRight;
    DirectX::XMFLOAT3    cbCameraForward;
    float cbTessellationFactor;
    int   cbWireframeOn;
    int   cbHeightMapOn;
    int   cbDiagType;
    float cbTexelCellU;
    float cbTexelCellV;
    float cbWorldCell;
};

struct RenderOption
{
    bool wireframeOn;
    bool diagModeOn;
    int heightMapOn;
    int  tessellateFactor;
    DiagType diagType;

    RenderOption::RenderOption()
        : wireframeOn(false)
        , diagModeOn(false)
        , heightMapOn(1)
        , tessellateFactor(10)
        , diagType(eDiagNormal)
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

    ID3D11BlendState*         mpBSalphaToCoverage = nullptr;
    ID3D11BlendState*         mpBStransparent = nullptr;

    ID3D11SamplerState*        mpSamplerLinear = nullptr;

    ID3D11ShaderResourceView*  mpHeightMapSRV  = nullptr;
    ID3D11ShaderResourceView*  mpEnvMapSRV  = nullptr;
    ID3D11ShaderResourceView*  mpSkyMapSRV  = nullptr;

public:
    void DestroyD3D11Objects()
    {
        SAFE_RELEASE( mpcbFrameParam );

        SAFE_RELEASE( mpControlPointIB );
        SAFE_RELEASE( mpControlPointVB );

        SAFE_RELEASE( mpRSWireframe );
        SAFE_RELEASE( mpRSSolid );

        SAFE_RELEASE( mpSamplerLinear  );

        SAFE_RELEASE( mpBSalphaToCoverage );
        SAFE_RELEASE( mpBStransparent );

        SAFE_RELEASE( mpHeightMapSRV   );
        SAFE_RELEASE( mpEnvMapSRV );
        SAFE_RELEASE( mpSkyMapSRV );

        ShaderManager::getShaderManager().Destroy();
    }

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