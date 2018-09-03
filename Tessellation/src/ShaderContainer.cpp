
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
        sprintf_s(buf, 512, "- Error @%s:%d\t  %s %#08x\t \n",__FILE__,__LINE__, #x, (ret) );  \
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
    const D3D11_INPUT_ELEMENT_DESC patchlayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    ID3DBlob* pErrorBlob = nullptr;

    ID3DBlob* pBlobVS = nullptr;
    ID3D11VertexShader*   pVertexShader = nullptr;
    ID3D11InputLayout*    pPatchLayout = nullptr;
    COMPILE_SHADER_CALL_CHECK(D3DCompileFromFile(mShaderFile.c_str(), nullptr, nullptr, "VSMain", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlobVS, &pErrorBlob));
    CREATE_SHADER_CALL_CHECK(pd3dDevice->CreateVertexShader(pBlobVS->GetBufferPointer(), pBlobVS->GetBufferSize(), nullptr, &pVertexShader));
    mVertexShaderList["PlainVertexShader"] = pVertexShader;
    CREATE_SHADER_CALL_CHECK(pd3dDevice->CreateInputLayout(patchlayout, ARRAYSIZE(patchlayout), pBlobVS->GetBufferPointer(), pBlobVS->GetBufferSize(), &pPatchLayout));
    mInputLayoutList["ControlPointLayout"] = pPatchLayout;
    pPatchLayout = nullptr;

    ID3DBlob* pBlobHS = nullptr;
    ID3D11HullShader* pHullShaderInteger = nullptr;
    COMPILE_SHADER_CALL_CHECK(D3DCompileFromFile(mShaderFile.c_str(), nullptr, nullptr, "HSMain", "hs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlobHS, &pErrorBlob));
    CREATE_SHADER_CALL_CHECK(pd3dDevice->CreateHullShader(pBlobHS->GetBufferPointer(), pBlobHS->GetBufferSize(), nullptr, &pHullShaderInteger));
    mHullShaderList["HullShader"] = pHullShaderInteger;
    pHullShaderInteger = nullptr;

    ID3DBlob* pBlobDS = nullptr;
    ID3D11DomainShader*   pDomainShader = nullptr;
    COMPILE_SHADER_CALL_CHECK(D3DCompileFromFile(mShaderFile.c_str(), nullptr, nullptr, "DSMain", "ds_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlobDS, &pErrorBlob));
    CREATE_SHADER_CALL_CHECK(pd3dDevice->CreateDomainShader(pBlobDS->GetBufferPointer(), pBlobDS->GetBufferSize(), nullptr, &pDomainShader));
    mDomainShaderList["DomainShader"] = pDomainShader;
    pDomainShader = nullptr;

    ID3DBlob* pBlobPS = nullptr;
    ID3D11PixelShader* pPlainPS = nullptr;
    COMPILE_SHADER_CALL_CHECK(D3DCompileFromFile(mShaderFile.c_str(), nullptr, nullptr, "PSMain", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlobPS, &pErrorBlob));
    CREATE_SHADER_CALL_CHECK(pd3dDevice->CreatePixelShader(pBlobPS->GetBufferPointer(), pBlobPS->GetBufferSize(), nullptr, &pPlainPS));
    mPixelShaderList["PlainPixelShader"] = pPlainPS;
    pPlainPS = nullptr;

    ID3DBlob* pBlobDiagPS= nullptr;
    ID3D11PixelShader*  pDiagPS = nullptr;
    COMPILE_SHADER_CALL_CHECK(D3DCompileFromFile(mShaderFile.c_str(), nullptr, nullptr, "DiagPSMain", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlobDiagPS, &pErrorBlob));
    CREATE_SHADER_CALL_CHECK(pd3dDevice->CreatePixelShader(pBlobDiagPS->GetBufferPointer(), pBlobDiagPS->GetBufferSize(), nullptr, &pDiagPS));
    mPixelShaderList["DiagPixelShader"] = pDiagPS;
    pDiagPS = nullptr;

#if 0
    ID3DBlob* pBlobGS = nullptr;
    ID3D11GeometryShader* pGeometryShader = nullptr;
    COMPILE_SHADER_CALL_CHECK(D3DCompileFromFile(mShaderFile.c_str(), nullptr, nullptr, "GSMain", "gs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlobGS, &pErrorBlob));
    CREATE_SHADER_CALL_CHECK(pd3dDevice->CreateGeometryShader(pBlobGS->GetBufferPointer(), pBlobGS->GetBufferSize(), nullptr, &pGeometryShader));
    mGeometryShaderList["GeometryShader"] = pGeometryShader;
    SAFE_RELEASE(pBlobGS);
#endif

#if 1
    SAFE_RELEASE(pBlobVS);
    SAFE_RELEASE(pBlobHS);
    SAFE_RELEASE(pBlobDS);
    SAFE_RELEASE(pBlobPS);
    SAFE_RELEASE(pBlobDiagPS);
#endif
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

static ShaderContainer* shaderContainer = nullptr;
ShaderContainer& ShaderContainer::getShaderContainer()
{
    if (!shaderContainer) shaderContainer = new ShaderContainer();
    return *shaderContainer;
}
