#ifndef D3D11_EFFECTS_H_
#define D3D11_EFFECTS_H_
#include "DXUT.h"
#include "SDKmisc.h"

#include <string>

// It is just used to record effect infomation.
// 1. vertex shader object
// 2. pixel shader object.
// 3. vertex attribute layout.
// 4. vertex format.
//
// It is the caller's responsiblity of creating and updating GPU Buffers.
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

    ~D3D11Effect()
    { }

    void Destroy()
    {
        SAFE_RELEASE(mpVertexLayout);
        SAFE_RELEASE(mpPixelShader);
        SAFE_RELEASE(mpVertexShader);
    }

    HRESULT InitEffect(ID3D11Device* pd3dDevice, LPCWSTR filename, LPCSTR VSentrypoint, LPCSTR PSentrypoint);
    HRESULT BindeBuffers(ID3D11DeviceContext* pd3dImmediateContext, ID3D11Buffer *ib, ID3D11Buffer* vbs[], UINT strides[], UINT offsets[], ID3D11Buffer *cb = nullptr);
    HRESULT ApplyEffect(ID3D11DeviceContext* pd3dImmediateContext, int indexCount, D3D_PRIMITIVE_TOPOLOGY primitivetopology);

};
#endif