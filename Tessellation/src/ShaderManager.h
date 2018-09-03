#ifndef  SHADER_MANAGER_H
#define  SHADER_MANAGER_H

#include <vector>
#include <map>
#include <string>
#include <d3d11.h>
#include <DirectXMath.h>

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p) = nullptr; } }
#endif

class ShaderManager
{
private:

public:
    ShaderManager()
    { }

    ~ShaderManager()
    { }

    static ShaderManager& getShaderManager();

    HRESULT InitD3D11ShaderObjects(ID3D11Device*  pd3dDevice);

    void Destroy();

public:

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
    std::map<std::string, ID3D11InputLayout*>  mInputLayoutList;

    
};

#endif