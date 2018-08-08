#ifndef  SHADER_CONTAINER_H
#define  SHADER_CONTAINER_H

#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p) = nullptr; } }
#endif


class ShaderContainer;

class Shader
{
    friend class ShaderContainer;

private:
    std::wstring mShaderFile;
    bool initialized;

public:
    Shader(std::wstring shaderfile)
        : mShaderFile(shaderfile)
        , initialized (false)
    { }

    ~Shader()
    { }

    void Init(ID3D11Device*  pd3dDevice)
    {
        if (initialized == false)
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
        return mVertexShaderList[name];
    }

    ID3D11HullShader*     getHullShader(std::string name)
    {
        return mHullShaderList[name];
    }

    ID3D11DomainShader*   getDomainShader(std::string name)
    {
        return mDomainShaderList[name];
    }

    ID3D11GeometryShader*   getGeometryShader(std::string name)
    {
        return mGeometryShaderList[name];
    }

    ID3D11PixelShader*    getPixelShader(std::string name)
    {
        return mPixelShaderList[name];
    }

    ID3D11InputLayout*    getInputLayout(std::string name)
    {
        return mInputLayoutList[name];
    }

private:
    
    std::map<std::string, ID3D11VertexShader*> mVertexShaderList;
    std::map<std::string, ID3D11PixelShader*>  mPixelShaderList;
    std::map<std::string, ID3D11HullShader*>   mHullShaderList;
    std::map<std::string, ID3D11DomainShader*> mDomainShaderList;
    std::map<std::string, ID3D11GeometryShader*> mGeometryShaderList;
    std::map<std::string, ID3D11InputLayout*>  mInputLayoutList;
    
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