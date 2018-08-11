#include "BezierSurface.h"
#include "CameraLayer.h"
#include "Logger.h"
#include "IDataSource.h"
#include "ShaderContainer.h"
#include <windows.h>
#include <d3d11.h>   
#include <DirectXMath.h>

#pragma warning( disable : 4100 )

using namespace DirectX;


static XMMATRIX   tempWorld(1, 0, 0, 0,
    0, 0, -1, 0,
    0, 1, 0, 0,
    0, 0, 0, 1);

class App
{
public:
    virtual bool Initialize(HWND hWnd)
    {
        HRESULT hr = S_OK;

        RECT rc;
        GetClientRect(hWnd, &rc);
        unsigned int width = rc.right - rc.left;
        unsigned int height = rc.bottom - rc.top;

        UINT createDeviceFlags = 0;

#ifdef _DEBUG
        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        D3D_DRIVER_TYPE driverTypes[] =
        {
            D3D_DRIVER_TYPE_HARDWARE,
            D3D_DRIVER_TYPE_WARP,
            D3D_DRIVER_TYPE_REFERENCE,
        };
        UINT numDriverTypes = ARRAYSIZE(driverTypes);

        D3D_FEATURE_LEVEL featureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
        };
        UINT numFeatureLevels = ARRAYSIZE(featureLevels);

        D3D_DRIVER_TYPE         driverType = D3D_DRIVER_TYPE_NULL;
        D3D_FEATURE_LEVEL       featureLevel = D3D_FEATURE_LEVEL_11_0;

        DXGI_SWAP_CHAIN_DESC sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = 1;
        sd.BufferDesc.Width = width;
        sd.BufferDesc.Height = height;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;

        // Create Device, DeviceContext, SwapChain, FeatureLevel
        for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
        {
            driverType = driverTypes[driverTypeIndex];
            hr = D3D11CreateDeviceAndSwapChain(NULL, driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &sd, &m_pSwapChain, &m_pd3dDevice, &featureLevel, &m_pImmediateContext);
            if (SUCCEEDED(hr)) break;
        }
        if (FAILED(hr))
        {
            Logger::getLogger() << "- Create D3D Device and Swap Chain Failed." << "\n" << std::endl;
            return false;
        }
#if 0
        // Create Render Target View Object from SwapChain's Back Buffer.
        // access one of swap chain's back buffer.[0-based buffer index, interface type which manipulates buffer, output param]
        ID3D11Texture2D* pBackBuffer = NULL;
        hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
        if (FAILED(hr))
        {
            Logger::getLogger() << "- Get Back Buffer from SwapChain Failed." << "\n";
            return false;
        }
        hr = m_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &m_pRenderTargetView);
        pBackBuffer->Release();
        if (FAILED(hr))
        {
            Logger::getLogger() << "- Create render target from Back buffer failed.\n" << "\n";
            return false;
        }
#else

        Resize(hWnd);
#endif


        m_pImmediateContext->OMSetRenderTargets(1, &m_pRenderTargetView, NULL);

        ////////////////////////////////////////////////////////////////////////
        /// Const Buffer
        ////////////////////////////////////////////////////////////////////////
        D3D11_BUFFER_DESC Desc;
        Desc.Usage = D3D11_USAGE_DYNAMIC;
        Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        Desc.MiscFlags = 0;
        Desc.ByteWidth = (sizeof(FrameParam) + 15) & ~0xf;
        m_pd3dDevice->CreateBuffer(&Desc, nullptr, &mpcbFrameParam);

        ////////////////////////////////////////////////////////////////////////
        /// Shader Container Initialize
        ////////////////////////////////////////////////////////////////////////
        ShaderContainer &container = ShaderContainer::getShaderContainer();
        //container.addShader(".\\shader\\drawCameraVector.hlsl");
        container.addShader(".\\shader\\TesseQuad.hlsl");
        container.Init(m_pd3dDevice);
        return true;
    }

    void Resize(HWND hWnd)
    {
        if (m_pd3dDevice == NULL)
            return;

        HRESULT hr = S_OK;
        RECT    rc;
        GetClientRect(hWnd, &rc);
        unsigned int width = rc.right - rc.left;
        unsigned int height = rc.bottom - rc.top;

        // release references to back buffer before resize, else fails
        SAFE_RELEASE(m_pRenderTargetView);

        DXGI_SWAP_CHAIN_DESC sd;
        m_pSwapChain->GetDesc(&sd);

        hr = m_pSwapChain->ResizeBuffers(sd.BufferCount, width, height, sd.BufferDesc.Format, 0);

        ID3D11Texture2D* pTexture;
        hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pTexture);
        hr = m_pd3dDevice->CreateRenderTargetView(pTexture, NULL, &m_pRenderTargetView);
        pTexture->Release();
        pTexture = NULL;

        D3D11_VIEWPORT vp;
        vp.Width  = width;
        vp.Height = height;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = 0.f;
        vp.TopLeftY = 0.f;
        m_pImmediateContext->RSSetViewports(1, &vp);
    }

protected:
    ID3D11Device*               m_pd3dDevice = nullptr;
    ID3D11DeviceContext*		m_pImmediateContext = nullptr;
    IDXGISwapChain*				m_pSwapChain = nullptr;
    ID3D11RenderTargetView*		m_pRenderTargetView = nullptr;
    ID3D11Buffer*               mpcbFrameParam = nullptr;
    float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

public:
    virtual void Render() = 0;

    virtual void SetMesh(IDataSource* source) = 0;

    virtual void Destory()
    {
        SAFE_RELEASE(m_pRenderTargetView);
        SAFE_RELEASE(m_pSwapChain);
        SAFE_RELEASE(m_pImmediateContext);
        SAFE_RELEASE(m_pd3dDevice);
    }

protected:
    void UpdateCBParam(ID3D11DeviceContext* pd3dImmediateContext)
    {
        Camera &camera = CameraManager::getCamera();
        const RenderOption & renderOption = RenderOption::getRenderOption();

        // WVP
        XMMATRIX mProj = camera.Proj();
        XMMATRIX mView = camera.View();
        XMMATRIX mViewProjection = mView * mProj;

        D3D11_MAPPED_SUBRESOURCE MappedResource;
        pd3dImmediateContext->Map(mpcbFrameParam, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
        auto pData = reinterpret_cast<FrameParam*>(MappedResource.pData);
        XMStoreFloat4x4(&pData->cbWorld, XMMatrixTranspose(tempWorld));
        XMStoreFloat4x4(&pData->cbViewProjection, XMMatrixTranspose(mViewProjection));
        //XMStoreFloat3(&pData->cbCameraPosWorld, camera.GetEyePt());
        pData->cbCameraPosWorld = camera.GetPosition();
        XMStoreFloat3(&pData->cbCameraUp, { 0.f, 1.f, 0.f });
        XMStoreFloat3(&pData->cbCameraRight, { 1.f, 0.f, 0.f });
        XMStoreFloat3(&pData->cbCameraForward, { 0.f, 0.f, 1.f });
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
};

class TessBezierSurface :public App
{
public:
    bool Initialize(HWND hWnd) override;
    void Render() override;
    void SetMesh(IDataSource* source)
    {
        surface.SetupMeshData(source);
    }

    void Destory() override
    {
        surface.DestroyD3D11Objects();
        App::Destory();
    }

private:
    BezierSurface surface;
};

class DrawCameraVector :public App
{
public:
    bool Initialize(HWND hWnd) override;
    void Render() override;

    void SetMesh(IDataSource* source)
    {
        layer.SetupMeshData(source);
    }

    void Destory() override
    {
        layer.DestroyD3D11Objects();
        App::Destory();
    }

private:
    CameraLayer layer;
};
