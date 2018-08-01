
#include "ShaderManager.h"              

//#define TESSE_SHADER_FILE L".\\shader\\TesseBezierSurface.hlsl"
//#define TESSE_SHADER_FILE L".\\shader\\TesseBezierSurface_tessFactor.hlsl"
#define TESSE_SHADER_FILE L".\\shader\\TesseQuad.hlsl"


ShaderManager& ShaderManager::getShaderManager()
{
    static ShaderManager* mgr = new ShaderManager();
    return *mgr;
}

HRESULT ShaderManager::InitD3D11ShaderObjects(ID3D11Device*  pd3dDevice)
{
    HRESULT hr;

    // Compile shaders
    ID3DBlob* pBlobVS = nullptr;
    ID3DBlob* pBlobHSInt = nullptr;
    ID3DBlob* pBlobDS = nullptr;
    ID3DBlob* pBlobPS = nullptr;
    ID3DBlob* pBlobPSSolid = nullptr;

    ID3D11VertexShader*   g_pVertexShader = nullptr;
    ID3D11HullShader*     g_pHullShaderInteger = nullptr;
    ID3D11DomainShader*   g_pDomainShader = nullptr;
    ID3D11PixelShader*    g_pSolidColorPS = nullptr;
    ID3D11PixelShader*    g_pWireframePS = nullptr;
    ID3D11InputLayout*    g_pPatchLayout = nullptr;

    const D3D11_INPUT_ELEMENT_DESC patchlayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    V_RETURN(DXUTCompileFromFile(TESSE_SHADER_FILE, nullptr, "BezierVS", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlobVS));
    V_RETURN(DXUTCompileFromFile(TESSE_SHADER_FILE, nullptr, "BezierHS", "hs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlobHSInt));
    V_RETURN(DXUTCompileFromFile(TESSE_SHADER_FILE, nullptr, "BezierDS", "ds_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlobDS));
    V_RETURN(DXUTCompileFromFile(TESSE_SHADER_FILE, nullptr, "WireframePS", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlobPS));
    V_RETURN(DXUTCompileFromFile(TESSE_SHADER_FILE, nullptr, "SolidColorPS", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlobPSSolid));

    // Create shaders
    V_RETURN(pd3dDevice->CreateVertexShader(pBlobVS->GetBufferPointer(), pBlobVS->GetBufferSize(), nullptr, &g_pVertexShader));
    DXUT_SetDebugName(g_pVertexShader, "BezierVS");

    V_RETURN(pd3dDevice->CreateInputLayout(patchlayout, ARRAYSIZE(patchlayout), pBlobVS->GetBufferPointer(), pBlobVS->GetBufferSize(), &g_pPatchLayout));
    DXUT_SetDebugName(g_pPatchLayout, "InputLayout");

    V_RETURN(pd3dDevice->CreateHullShader(pBlobHSInt->GetBufferPointer(), pBlobHSInt->GetBufferSize(), nullptr, &g_pHullShaderInteger));
    DXUT_SetDebugName(g_pHullShaderInteger, "BezierHS integer partition mode");

    V_RETURN(pd3dDevice->CreateDomainShader(pBlobDS->GetBufferPointer(), pBlobDS->GetBufferSize(), nullptr, &g_pDomainShader));
    DXUT_SetDebugName(g_pDomainShader, "BezierDS");

    V_RETURN(pd3dDevice->CreatePixelShader(pBlobPSSolid->GetBufferPointer(), pBlobPSSolid->GetBufferSize(), nullptr, &g_pSolidColorPS));
    DXUT_SetDebugName(g_pSolidColorPS, "SolidColorPS");

    V_RETURN(pd3dDevice->CreatePixelShader(pBlobPS->GetBufferPointer(), pBlobPS->GetBufferSize(), nullptr, &g_pWireframePS));
    DXUT_SetDebugName(g_pSolidColorPS, "WireframePS");

    mVertexShaderList["PlainVertexShader"] = g_pVertexShader;
    mInputLayoutList["ControlPointLayout"] = g_pPatchLayout;
    mPixelShaderList["PlainPixelShader"] = g_pSolidColorPS;
    mPixelShaderList["WireframePixelShader"] = g_pWireframePS;
    mHullShaderList["HullShader"] = g_pHullShaderInteger;
    mDomainShaderList["DomainShader"] = g_pDomainShader;

    SAFE_RELEASE(pBlobVS);
    SAFE_RELEASE(pBlobHSInt);
    SAFE_RELEASE(pBlobDS);
    SAFE_RELEASE(pBlobPS);
    SAFE_RELEASE(pBlobPSSolid);
}

void ShaderManager::Destroy()
{
    for (auto it = mVertexShaderList.begin(); it != mVertexShaderList.end(); ++it)
    {
        it->second->Release();
    }
    for (auto it = mPixelShaderList.begin(); it != mPixelShaderList.end(); ++it)
    {
        it->second->Release();
    }
    for (auto it = mHullShaderList.begin(); it != mHullShaderList.end(); ++it)
    {
        it->second->Release();
    }
    for (auto it = mDomainShaderList.begin(); it != mDomainShaderList.end(); ++it)
    {
        it->second->Release();
    }

    for (auto it = mInputLayoutList.begin(); it != mInputLayoutList.end(); ++it)
    {
        it->second->Release();
    }
    mVertexShaderList.clear();
    mPixelShaderList.clear();
    mHullShaderList.clear();
    mDomainShaderList.clear();
    mInputLayoutList.clear();
}

