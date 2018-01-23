#include "D3D11Effect.h"

HRESULT D3D11Effect::InitEffect(ID3D11Device* pd3dDevice, LPCWSTR filename, LPCSTR VSentrypoint, LPCSTR PSentrypoint)
{
    HRESULT hr = S_OK;
    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;

    // Disable optimizations to further improve shader debugging
    dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    // Compile the vertex shader
    // Create the vertex shader
    ID3DBlob* pVSBlob = nullptr;
    V_RETURN(DXUTCompileFromFile(filename, nullptr, VSentrypoint, "vs_4_0", dwShaderFlags, 0, &pVSBlob));
    hr = pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &mpVertexShader);
    if (FAILED(hr))
    {
        SAFE_RELEASE(pVSBlob);
        return hr;
    }

    if ((mVertexFormat & 0x1) && !(mVertexFormat & (~0x1)))
    {
        // Create the input layout
        D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        UINT numElements = ARRAYSIZE(layout);
        pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &mpVertexLayout);
        SAFE_RELEASE(pVSBlob);
        if (FAILED(hr)) return hr;
    }
    else if ((mVertexFormat & 0x1) && (mVertexFormat & 0x2) && (mVertexFormat & 0x4))
    {
        // Create the input layout
        D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        UINT numElements = ARRAYSIZE(layout);
        pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &mpVertexLayout);
        SAFE_RELEASE(pVSBlob);
        if (FAILED(hr)) return hr;
    }
    else
    {
        SAFE_RELEASE(pVSBlob);
        return hr;
    }

    // Compile the pixel shader
    // Create the pixel shader
    ID3DBlob* pPSBlob = nullptr;
    V_RETURN(DXUTCompileFromFile(filename, nullptr, PSentrypoint, "ps_4_0", dwShaderFlags, 0, &pPSBlob));
    hr = pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &mpPixelShader);
    SAFE_RELEASE(pPSBlob);
    return hr;
}

HRESULT D3D11Effect::BindeBuffers(ID3D11DeviceContext* pd3dImmediateContext, ID3D11Buffer *ib, ID3D11Buffer* vbs[], UINT strides[], UINT offsets[], ID3D11Buffer *cb)
{
    pd3dImmediateContext->VSSetConstantBuffers(0, 1, &cb);
    pd3dImmediateContext->PSSetConstantBuffers(0, 1, &cb);
    pd3dImmediateContext->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
    pd3dImmediateContext->IASetInputLayout(mpVertexLayout);
    if ((mVertexFormat & 0x1) && !(mVertexFormat & (~0x1)))
    {
        pd3dImmediateContext->IASetVertexBuffers(0, 1, vbs, strides, offsets);
    }
    else if ((mVertexFormat & 0x1) && (mVertexFormat & 0x2) && (mVertexFormat & 0x4))
    {
        pd3dImmediateContext->IASetVertexBuffers(0, 3, vbs, strides, offsets);
    }
    else
    {
        assert(0);
    }
    return S_OK;
}

HRESULT D3D11Effect::ApplyEffect(ID3D11DeviceContext* pd3dImmediateContext)
{
    pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    pd3dImmediateContext->VSSetShader(mpVertexShader, nullptr, 0);
    pd3dImmediateContext->PSSetShader(mpPixelShader, nullptr, 0);
    pd3dImmediateContext->DrawIndexed(24, 0, 0);
    return S_OK;
}