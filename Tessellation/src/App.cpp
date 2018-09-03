#include "App.h"

HRESULT TessSurface::Initialize(HWND hWnd)
{
    App::Initialize(hWnd);
    surface.Initialize(this->m_pd3dDevice, this->m_pImmediateContext);

    ////////////////////////////////////////////////////////////////////////
    /// Shader Container Initialize
    ////////////////////////////////////////////////////////////////////////
    ShaderContainer &container = ShaderContainer::getShaderContainer();
    container.addShader(".\\shader\\TesseQuad.hlsl");
    container.Init(m_pd3dDevice);
    SetWindowText(hWnd, L"Initialized OK.");
    return 0;
}

void TessSurface::Render()
{
    m_pImmediateContext->ClearRenderTargetView(m_pRenderTargetView, ClearColor);
    surface.Render(m_pImmediateContext);
}

HRESULT DrawCameraVector::Initialize(HWND hWnd)
{
    App::Initialize(hWnd);
    layer.Initialize(this->m_pd3dDevice, this->m_pImmediateContext);

    ////////////////////////////////////////////////////////////////////////
    /// Shader Container Initialize
    ////////////////////////////////////////////////////////////////////////
    ShaderContainer &container = ShaderContainer::getShaderContainer();
    container.addShader(".\\shader\\drawCameraVector.hlsl");
    container.Init(m_pd3dDevice);

    SetWindowText(hWnd, L"Initialized OK.");
    return 0;
}

void DrawCameraVector::Render()
{
    m_pImmediateContext->ClearRenderTargetView(m_pRenderTargetView, ClearColor);
    layer.Render(this->m_pImmediateContext);
}