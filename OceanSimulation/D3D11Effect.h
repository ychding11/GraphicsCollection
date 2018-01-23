#ifndef D3D11_EFFECTS_H_
#define D3D11_EFFECTS_H_
#include "DXUT.h"
#include "SDKmisc.h"

#include <string>


class D3D11Effect
{
public:

    ID3D11VertexShader*    mpVertexShader = nullptr;
    ID3D11PixelShader*     mpPixelShader = nullptr;
    ID3D11InputLayout*     mpVertexLayout = nullptr;
    unsigned int mVertexFormat;

    D3D11Effect(unsigned int vertexFormat = 0x1)
        : mVertexFormat(vertexFormat)
    { }

    HRESULT InitEffect(ID3D11Device* pd3dDevice, LPCWSTR filename, LPCSTR VSentrypoint, LPCSTR PSentrypoint);
    HRESULT BindeBuffers(ID3D11Buffer *ib = nullptr, ID3D11Buffer *pb = nullptr, ID3D11Buffer * nb = nullptr, ID3D11Buffer* tb = nullptr, ID3D11Buffer *cb = nullptr);
    HRESULT ApplyEffect(ID3D11DeviceContext* pd3dImmediateContext);

    ~D3D11Effect()
    { }

};
#endif