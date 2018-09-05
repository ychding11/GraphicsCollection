#ifndef  SHADER_CONTAINER_H
#define  SHADER_CONTAINER_H

#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <regex>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p) = nullptr; } }
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

#define CHECK_D3D11_OBJECT_RETURN(x,y)                \
do{                                                   \
    if (x.find(y) != x.end())                         \
        return x[y];                                  \
    else                                              \
    {                                                 \
        char buf[512];                                \
        sprintf_s(buf, 512, "- Error @%s:%d\t  %s %s\t \n",__FILE__,__LINE__, #x, y.c_str());  \
        OutputDebugStringA(buf);                      \
        return nullptr;                               \
    }                                                 \
} while(0)


class ShaderContainer;

class Shader
{
    friend class ShaderContainer;

private:
    std::wstring mShaderFile;
    bool initialized;
    bool mHasGS;
    bool mHasHS;
    bool mHasDS;

public:
    Shader(std::wstring shaderfile)
        : mShaderFile(shaderfile)
        , initialized (false)
        , mHasGS(false)
        , mHasHS(false)
        , mHasDS(false)
    { }

    ~Shader()
    { }

    void Init(ID3D11Device*  pd3dDevice)
    {
        //if (initialized == false)
        {
            InitD3D11ShaderObjects( pd3dDevice);
            initialized = true;
        }
    }

    void Destroy();

public:

    HRESULT InitD3D11ShaderObjects(ID3D11Device*  pd3dDevice);

    ID3D11VertexShader*   getVertexShader(std::string name)
    {
#if 0
        if (mVertexShaderList.find(name) != mVertexShaderList.end())
            return mVertexShaderList[name];
#endif
        CHECK_D3D11_OBJECT_RETURN(mVertexShaderList,name);
    }

    ID3D11HullShader*     getHullShader(std::string name)
    {
        //return mHullShaderList[name];
        CHECK_D3D11_OBJECT_RETURN(mHullShaderList,name);
    }

    ID3D11DomainShader*   getDomainShader(std::string name)
    {
        //return mDomainShaderList[name];
        CHECK_D3D11_OBJECT_RETURN(mDomainShaderList,name);
    }

    ID3D11GeometryShader*   getGeometryShader(std::string name)
    {
        //return mGeometryShaderList[name];
        CHECK_D3D11_OBJECT_RETURN(mGeometryShaderList,name);
    }

    ID3D11PixelShader*    getPixelShader(std::string name)
    {
        //return mPixelShaderList[name];
        CHECK_D3D11_OBJECT_RETURN(mPixelShaderList,name);
    }

    ID3D11InputLayout*    getInputLayout(std::string name)
    {
        //return mInputLayoutList[name];
        CHECK_D3D11_OBJECT_RETURN(mInputLayoutList,name);
    }

private:
    
    std::map<std::string, ID3D11VertexShader*> mVertexShaderList;
    std::map<std::string, ID3D11PixelShader*>  mPixelShaderList;
    std::map<std::string, ID3D11HullShader*>   mHullShaderList;
    std::map<std::string, ID3D11DomainShader*> mDomainShaderList;
    std::map<std::string, ID3D11GeometryShader*> mGeometryShaderList;
    std::map<std::string, ID3D11InputLayout*>    mInputLayoutList;
    
    void AnalyzeShaderComponent();
};

class ShaderContainer
{
private:
    std::vector<Shader> mContainer;

public:

    static ShaderContainer& getShaderContainer();

    Shader& operator[](std::string name)
    {
        std::wstring str;
        str.assign(name.begin(), name.end());
        for (auto it = mContainer.begin(); it != mContainer.end(); ++it)
        {
            if (it->mShaderFile == str)
                return *it;
        }
        std::cout << "- no matched!" << std::endl;
        exit(1);
    }

    void addShader(std::string shadername)
    {
        std::wstring str;
        str.assign(shadername.begin(), shadername.end());

        for (auto it = mContainer.begin(); it != mContainer.end(); ++it)
        {
            if (it->mShaderFile == str)
                return;
        }
        mContainer.push_back(Shader(str));
    }

    void Init(ID3D11Device*  pd3dDevic)
    {
        for (auto it = mContainer.begin(); it != mContainer.end(); ++it)
        {
            it->Init( pd3dDevic);
        }
    }

    void Destory()
    {
        for (auto it = mContainer.begin(); it != mContainer.end(); ++it)
        {
            it->Destroy();
        }
    }
};

#endif