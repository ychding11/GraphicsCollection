#include "App.h"

bool TessBezierSurface::Initialize(HWND hWnd)
{
    App::Initialize(hWnd);
    surface.Initialize(this->m_pd3dDevice, this->m_pImmediateContext);
    return true;
}

void TessBezierSurface::Render()
{
    //float ClearColor[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    m_pImmediateContext->ClearRenderTargetView(m_pRenderTargetView, ClearColor);
    surface.Render(this->m_pImmediateContext);
}

bool DrawCameraVector::Initialize(HWND hWnd)
{
    App::Initialize(hWnd);
    layer.Initialize(this->m_pd3dDevice, this->m_pImmediateContext);
    return true;
}

void DrawCameraVector::Render()
{
    //float ClearColor[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    m_pImmediateContext->ClearRenderTargetView(m_pRenderTargetView, ClearColor);
    layer.Render(this->m_pImmediateContext);
}