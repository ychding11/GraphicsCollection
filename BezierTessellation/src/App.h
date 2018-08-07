#include "resource.h"
#include "BezierSurface.h"
#include "IDataSource.h"
#include "ShaderContainer.h"
#include "CameraLayer.h"

#pragma warning( disable : 4100 )

using namespace DirectX;


class App
{
protected: 
    ID3D11Device* pd3dDevice;

public:
    virtual bool Initialize()
    {

    }

    virtual void Render() = 0;
};

class TessBezierSurface :public App
{
public:
    void Render() override;
};
