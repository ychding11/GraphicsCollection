
#include "ShaderContainer.h"              


#undef V
#define V(x)           { hr = (x); }
#undef  V_RETURN
#define V_RETURN(x)    { hr = (x); if( FAILED(hr) ) { return hr; } }

#if defined(PROFILE) || defined(DEBUG)
inline void SetDebugName(_In_ IDXGIObject* pObj, _In_z_ const CHAR* pstrName)
{
    if (pObj)
        pObj->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen(pstrName), pstrName);
}
inline void SetDebugName(_In_ ID3D11Device* pObj, _In_z_ const CHAR* pstrName)
{
    if (pObj)
        pObj->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen(pstrName), pstrName);
}
inline void SetDebugName(_In_ ID3D11DeviceChild* pObj, _In_z_ const CHAR* pstrName)
{
    if (pObj)
        pObj->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen(pstrName), pstrName);
}
#else
#define SetDebugName( pObj, pstrName )
#endif

#define COMPILE_SHADER_CALL_CHECK(x)                  \
do{                                                   \
    LRESULT ret = x;                                  \
    if((ret) != S_OK)                                 \
    {                                                 \
        char buf[512];                                \
        sprintf_s(buf, 512, "- Error @%s:%d\t  %s %d\t \n",__FILE__,__LINE__, #x, (ret) );  \
        OutputDebugStringA(buf);                      \
        if (pErrorBlob) OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());    \
        SAFE_RELEASE(pErrorBlob);                     \
    }                                                 \
} while(0)

#define CREATE_SHADER_CALL_CHECK(x)                   \
do{                                                   \
    LRESULT ret = x;                                  \
    if((ret) != S_OK)                                 \
    {                                                 \
        char buf[512];                                \
        sprintf_s(buf, 512, "- Error @%s:%d\t  %s %d\t \n",__FILE__,__LINE__, #x, (ret) );  \
        OutputDebugStringA(buf);                      \
    }                                                 \
} while(0)


HRESULT Shader::InitD3D11ShaderObjects(ID3D11Device*  pd3dDevice)
{
    HRESULT hr;

    // Compile shaders
    ID3DBlob* pBlobVS = nullptr;
    ID3DBlob* pBlobHS = nullptr;
    ID3DBlob* pBlobDS = nullptr;
    ID3DBlob* pBlobGS = nullptr;
    ID3DBlob* pBlobDiagPS= nullptr;
    ID3DBlob* pBlobPS = nullptr;
    ID3DBlob* pErrorBlob = nullptr;

    ID3D11VertexShader*   g_pVertexShader = nullptr;
    ID3D11HullShader*     g_pHullShaderInteger = nullptr;
    ID3D11DomainShader*   g_pDomainShader = nullptr;
    ID3D11PixelShader*    g_pPlainPS = nullptr;
    ID3D11PixelShader*    g_pDiagPS = nullptr;
    ID3D11InputLayout*    g_pPatchLayout = nullptr;
    ID3D11GeometryShader* g_pGeometryShader = nullptr;

    const D3D11_INPUT_ELEMENT_DESC patchlayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    COMPILE_SHADER_CALL_CHECK(D3DCompileFromFile(mShaderFile.c_str(), nullptr, nullptr, "VSMain", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlobVS, &pErrorBlob));

    COMPILE_SHADER_CALL_CHECK(D3DCompileFromFile(mShaderFile.c_str(), nullptr, nullptr, "HSMain", "hs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlobHS, &pErrorBlob));

    COMPILE_SHADER_CALL_CHECK(D3DCompileFromFile(mShaderFile.c_str(), nullptr, nullptr, "DSMain", "ds_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlobDS, &pErrorBlob));

    COMPILE_SHADER_CALL_CHECK(D3DCompileFromFile(mShaderFile.c_str(), nullptr, nullptr, "GSMain", "gs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlobGS, &pErrorBlob));

    COMPILE_SHADER_CALL_CHECK(D3DCompileFromFile(mShaderFile.c_str(), nullptr, nullptr, "PSMain", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlobPS, &pErrorBlob));

    COMPILE_SHADER_CALL_CHECK(D3DCompileFromFile(mShaderFile.c_str(), nullptr, nullptr, "DiagPSMain", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlobDiagPS, &pErrorBlob));

    // Create shaders
    CREATE_SHADER_CALL_CHECK(pd3dDevice->CreateVertexShader(pBlobVS->GetBufferPointer(), pBlobVS->GetBufferSize(), nullptr, &g_pVertexShader));
    SetDebugName(g_pVertexShader, "VS");
    mVertexShaderList["PlainVertexShader"] = g_pVertexShader;

    CREATE_SHADER_CALL_CHECK(pd3dDevice->CreateInputLayout(patchlayout, ARRAYSIZE(patchlayout), pBlobVS->GetBufferPointer(), pBlobVS->GetBufferSize(), &g_pPatchLayout));
    SetDebugName(g_pPatchLayout, "InputLayout");
    mInputLayoutList["ControlPointLayout"] = g_pPatchLayout;

    CREATE_SHADER_CALL_CHECK(pd3dDevice->CreateHullShader(pBlobHS->GetBufferPointer(), pBlobHS->GetBufferSize(), nullptr, &g_pHullShaderInteger));
    SetDebugName(g_pHullShaderInteger, "HS");
    mHullShaderList["HullShader"] = g_pHullShaderInteger;

    CREATE_SHADER_CALL_CHECK(pd3dDevice->CreateDomainShader(pBlobDS->GetBufferPointer(), pBlobDS->GetBufferSize(), nullptr, &g_pDomainShader));
    SetDebugName(g_pDomainShader, "DS");
    mDomainShaderList["DomainShader"] = g_pDomainShader;

    CREATE_SHADER_CALL_CHECK(pd3dDevice->CreateGeometryShader(pBlobGS->GetBufferPointer(), pBlobGS->GetBufferSize(), nullptr, &g_pGeometryShader));
    SetDebugName(g_pGeometryShader, "GS");
    mGeometryShaderList["GeometryShader"] = g_pGeometryShader;

    CREATE_SHADER_CALL_CHECK(pd3dDevice->CreatePixelShader(pBlobPS->GetBufferPointer(), pBlobPS->GetBufferSize(), nullptr, &g_pPlainPS));
    SetDebugName(g_pPlainPS, "PlainPixelShader");
    mPixelShaderList["PlainPixelShader"] = g_pPlainPS;

    CREATE_SHADER_CALL_CHECK(pd3dDevice->CreatePixelShader(pBlobDiagPS->GetBufferPointer(), pBlobDiagPS->GetBufferSize(), nullptr, &g_pDiagPS));
    SetDebugName(g_pDiagPS, "DiagPS");
    mPixelShaderList["DiagPixelShader"] = g_pDiagPS;

    g_pVertexShader = nullptr;
    g_pHullShaderInteger = nullptr;
    g_pDomainShader = nullptr;
    g_pGeometryShader = nullptr;
    g_pPlainPS = nullptr;
    g_pDiagPS = nullptr;
    g_pPatchLayout = nullptr;

    SAFE_RELEASE(pBlobVS);
    SAFE_RELEASE(pBlobHS);
    SAFE_RELEASE(pBlobDS);
    SAFE_RELEASE(pBlobGS);
    SAFE_RELEASE(pBlobPS);
    SAFE_RELEASE(pBlobDiagPS);
    return 0;
}

void Shader::Destroy()
{
    for (auto it = mVertexShaderList.begin(); it != mVertexShaderList.end(); ++it)
    {
        it->second->Release();
        it->second = nullptr;
    }
    for (auto it = mPixelShaderList.begin(); it != mPixelShaderList.end(); ++it)
    {
        it->second->Release();
        it->second = nullptr;
    }
    for (auto it = mHullShaderList.begin(); it != mHullShaderList.end(); ++it)
    {
        it->second->Release();
        it->second = nullptr;
    }
    for (auto it = mDomainShaderList.begin(); it != mDomainShaderList.end(); ++it)
    {
        it->second->Release();
        it->second = nullptr;
    }

    for (auto it = mInputLayoutList.begin(); it != mInputLayoutList.end(); ++it)
    {
        it->second->Release();
        it->second = nullptr;
    }
    mVertexShaderList.clear();
    mPixelShaderList.clear();
    mHullShaderList.clear();
    mDomainShaderList.clear();
    mInputLayoutList.clear();
}

static ShaderContainer shaderContainer;
ShaderContainer& ShaderContainer::getShaderContainer()
{
    return shaderContainer;
}
