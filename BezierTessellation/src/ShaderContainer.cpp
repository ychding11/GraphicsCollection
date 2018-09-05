
#include "ShaderContainer.h"              



HRESULT Shader::InitD3D11ShaderObjects(ID3D11Device*  pd3dDevice)
{      
    UINT Flags1 = D3DCOMPILE_ENABLE_STRICTNESS;

#if defined( DEBUG ) || defined( _DEBUG )
    Flags1 |= D3DCOMPILE_DEBUG;
#endif
    const D3D11_INPUT_ELEMENT_DESC patchlayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    ID3DBlob* pErrorBlob = nullptr;

    AnalyzeShaderComponent();

    ID3DBlob* pBlobVS = nullptr;
    ID3D11VertexShader*   pVertexShader = nullptr;
    ID3D11InputLayout*    pPatchLayout = nullptr;
    COMPILE_SHADER_CALL_CHECK(D3DCompileFromFile(mShaderFile.c_str(), nullptr, nullptr, "VSMain", "vs_5_0", Flags1, 0, &pBlobVS, &pErrorBlob));
    CREATE_SHADER_CALL_CHECK(pd3dDevice->CreateVertexShader(pBlobVS->GetBufferPointer(), pBlobVS->GetBufferSize(), nullptr, &pVertexShader));
    mVertexShaderList["PlainVertexShader"] = pVertexShader;
    CREATE_SHADER_CALL_CHECK(pd3dDevice->CreateInputLayout(patchlayout, ARRAYSIZE(patchlayout), pBlobVS->GetBufferPointer(), pBlobVS->GetBufferSize(), &pPatchLayout));
    mInputLayoutList["ControlPointLayout"] = pPatchLayout;
    pPatchLayout = nullptr;

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

    if (mHasHS)
    {
        ID3DBlob* pBlobHS = nullptr;
        ID3D11HullShader* pHullShaderInteger = nullptr;
        COMPILE_SHADER_CALL_CHECK(D3DCompileFromFile(mShaderFile.c_str(), nullptr, nullptr, "HSMain", "hs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlobHS, &pErrorBlob));
        CREATE_SHADER_CALL_CHECK(pd3dDevice->CreateHullShader(pBlobHS->GetBufferPointer(), pBlobHS->GetBufferSize(), nullptr, &pHullShaderInteger));
        mHullShaderList["HullShader"] = pHullShaderInteger;
        pHullShaderInteger = nullptr;
        SAFE_RELEASE(pBlobHS);
    }

    if (mHasDS)
    {
        ID3DBlob* pBlobDS = nullptr;
        ID3D11DomainShader*   pDomainShader = nullptr;
        COMPILE_SHADER_CALL_CHECK(D3DCompileFromFile(mShaderFile.c_str(), nullptr, nullptr, "DSMain", "ds_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlobDS, &pErrorBlob));
        CREATE_SHADER_CALL_CHECK(pd3dDevice->CreateDomainShader(pBlobDS->GetBufferPointer(), pBlobDS->GetBufferSize(), nullptr, &pDomainShader));
        mDomainShaderList["DomainShader"] = pDomainShader;
        pDomainShader = nullptr;
        SAFE_RELEASE(pBlobDS);
    }

#if 1
    if (mHasGS)
    {
        ID3DBlob* pBlobGS = nullptr;
        ID3D11GeometryShader* pGeometryShader = nullptr;
        COMPILE_SHADER_CALL_CHECK(D3DCompileFromFile(mShaderFile.c_str(), nullptr, nullptr, "GSMain", "gs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlobGS, &pErrorBlob));
        CREATE_SHADER_CALL_CHECK(pd3dDevice->CreateGeometryShader(pBlobGS->GetBufferPointer(), pBlobGS->GetBufferSize(), nullptr, &pGeometryShader));
        mGeometryShaderList["GeometryShader"] = pGeometryShader;
        pGeometryShader = nullptr;
        SAFE_RELEASE(pBlobGS);
    }
#endif

#if 1
    SAFE_RELEASE(pBlobVS);
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

void Shader::AnalyzeShaderComponent()
{
    std::string filename(mShaderFile.begin(),mShaderFile.end());
    std::ifstream infile(filename.c_str());
    std::string sLine;

    if (infile.good())
    {
        getline(infile, sLine);
        infile.close();
    }
    else
    {
        char buf[512];
        sprintf_s(buf, 512, "- Error @%s:%d\t  %s %s\t \n",__FILE__,__LINE__, __FUNCTION__, filename.c_str());
        OutputDebugStringA(buf);
        exit(1);
    }

    std::regex rgxVS("vs");
    std::regex rgxPS("ps");
    std::regex rgxHS("hs");
    std::regex rgxDS("ds");
    std::regex rgxGS("gs");

    if (std::regex_search(sLine, rgxHS))
    {
        mHasHS = true;
    }
    else
    {
        mHasHS = false;
    }

    if (std::regex_search(sLine, rgxDS))
    {
        mHasDS = true;
    }
    else
    {
        mHasDS = false;
    }

    if (std::regex_search(sLine, rgxGS))
    {
        mHasGS = true;
    }
    else
    {
        mHasGS = false;
    }
    if (std::regex_search(sLine, rgxVS))
    {
    }
    else
    {
        exit(1);
    }

    if (std::regex_search(sLine, rgxPS))
    {
    }
    else
    {
        exit(1);
    }

}

ShaderContainer& ShaderContainer::getShaderContainer()
{
    static ShaderContainer* shaderContainer = new ShaderContainer();
    return *shaderContainer;
}
