#ifndef  CAMERA_LAYER_H 
#define  CAMERA_LAYER_H 

#include <vector>
#include <d3d11.h>
#include <DirectXMath.h>
#include "ShaderManager.h"
#include "IDataSource.h"


#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p) = nullptr; } }
#endif

class CameraLayer 
{
public:

    static CameraLayer& getCameraLayer();

private:
    IDataSource*  mMeshData;

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
    }

public:
    CameraLayer (void)
    { }

    ~CameraLayer()
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

#endif  