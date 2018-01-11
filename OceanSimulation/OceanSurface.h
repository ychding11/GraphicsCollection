#include "DXUT.h"
#include "DXUTcamera.h"
#include "SDKmisc.h"
#include "DDSTextureLoader.h"
#include "PlaneMesh.h"

#pragma warning( disable : 4100 )

using namespace DirectX;

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------

class OceanSurface
{
public:
    struct CBChangesEveryFrame
    {
        XMFLOAT4X4 mWorldViewProj;
        XMFLOAT4X4 mInvProjectionView;
        XMFLOAT4X4 mWorld;
        XMFLOAT4   vMeshColor;
    };

public:
    XMMATRIX                mmWorld;
    XMMATRIX                mmView;
    XMMATRIX                mmProjection;
    XMMATRIX                mmInvView;
    XMMATRIX                mmInvProjection;

    ID3D11VertexShader*     mpVertexShader = nullptr;
    ID3D11PixelShader*      mpPixelShader = nullptr;
    ID3D11InputLayout*      mpVertexLayout = nullptr;
    ID3D11Buffer*			mpIndexBuffer  = nullptr;
    ID3D11Buffer*			mpVertexBuffer = nullptr;
    ID3D11Buffer*           mpCBChangesEveryFrame = nullptr;
    PlaneMesh               mPlane;
    LPCWSTR                 mEffects;

    XMVECTOR points[24];
    XMVECTOR mUpper;
    XMVECTOR mLow;
    XMVECTOR mBase;
    int nPoints = 0;
    float mFOV;
    float mAspect;
	float mXspan;
	float mYspan;

public:
    OceanSurface()
        : mPlane(32, 32)
        , mEffects(L"oceanSimulation.fx")
    {

        mmWorld = XMMatrixIdentity();
        mmView  = XMMatrixIdentity();
        mmProjection = XMMatrixIdentity();
    }

    ~OceanSurface()
    {
        Destroy();
    }


    void Destroy()
    {
        SAFE_RELEASE( mpVertexLayout );
        SAFE_RELEASE( mpIndexBuffer);
        SAFE_RELEASE( mpVertexBuffer);
        SAFE_RELEASE( mpCBChangesEveryFrame);
        SAFE_RELEASE( mpVertexShader);
        SAFE_RELEASE( mpPixelShader);
    }

public:
    HRESULT Init(ID3D11Device* pd3dDevice, void* pUserContext)
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
        V_RETURN(DXUTCompileFromFile(mEffects, nullptr, "VS", "vs_4_0", dwShaderFlags, 0, &pVSBlob));
        hr = pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &mpVertexShader);
        if (FAILED(hr))
        {
            SAFE_RELEASE(pVSBlob);
            return hr;
        }

        // Create the input layout
        D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        UINT numElements = ARRAYSIZE(layout);
        pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &mpVertexLayout);
        SAFE_RELEASE(pVSBlob);
        if (FAILED(hr)) return hr;

        // Compile the pixel shader
        // Create the pixel shader
        ID3DBlob* pPSBlob = nullptr;
        V_RETURN(DXUTCompileFromFile(mEffects, nullptr, "PS", "ps_4_0", dwShaderFlags, 0, &pPSBlob));
        hr = pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &mpPixelShader);
        SAFE_RELEASE(pPSBlob);
        if (FAILED(hr)) return hr;
        return CreateAndUpdatePlaneMeshBuffer(pd3dDevice);
    }

public:

    // --------------------------------------------------------------------------------------
    // CreateAndUpdatePlaneMeshBuffer, create buffer every frame.
    //--------------------------------------------------------------------------------------
    HRESULT CreateAndUpdatePlaneMeshBuffer(ID3D11Device* pd3dDevice )
    {
        HRESULT  hr;
        D3D11_BUFFER_DESC bd;
        ZeroMemory(&bd, sizeof(bd));
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = 0;
        bd.ByteWidth = mPlane.mVertexBuffer.size() * sizeof(PlaneVertex);

        D3D11_SUBRESOURCE_DATA InitData;
        ZeroMemory(&InitData, sizeof(InitData));
        InitData.pSysMem = &(mPlane.mVertexBuffer[0]);
        SAFE_RELEASE(mpVertexBuffer);
        V_RETURN(pd3dDevice->CreateBuffer(&bd, &InitData, &mpVertexBuffer));

        bd.ByteWidth = mPlane.mIndexBuffer.size() * sizeof(int);
        bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        ZeroMemory(&InitData, sizeof(InitData));
        InitData.pSysMem = &(mPlane.mIndexBuffer[0]);
        SAFE_RELEASE(mpIndexBuffer);
        V_RETURN(pd3dDevice->CreateBuffer(&bd, &InitData, &mpIndexBuffer));

        // Create the constant buffers
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        bd.ByteWidth = sizeof(CBChangesEveryFrame);
        V_RETURN(pd3dDevice->CreateBuffer(&bd, nullptr, &mpCBChangesEveryFrame));
        return S_OK;
    }

    void UpdateParameters(const CModelViewerCamera  &camera )
    {
        mmView       = camera.GetViewMatrix();
        mmProjection = camera.GetProjMatrix();
        CModelViewerCamera temp(camera);
        const XMVECTORF32 delt = { 0.0f, 1.0f, 0.0f, 0.f };
        temp.SetViewParams(camera.GetEyePt() + delt, camera.GetLookAtPt());
        mmInvView = XMMatrixInverse(0, temp.GetViewMatrix());
        mmInvProjection = XMMatrixInverse(0, temp.GetProjMatrix());

    }

    void UpdateParameters(const XMMATRIX &view, const XMMATRIX &proj )
    {
        mmView = view;
        mmProjection = proj;
        mmInvView = XMMatrixInverse(0, mmView);
        mmInvProjection = XMMatrixInverse(0, mmProjection);
    }

    void UpdateParameters(ID3D11DeviceContext* pd3dImmediateContext)
    {
        HRESULT hr;
        XMMATRIX mWorldViewProjection = mmWorld * mmView * mmProjection;
        XMMATRIX mInvProjectionView   = mmInvProjection * mmInvView;

        // Update constant buffer that changes once per frame
        D3D11_MAPPED_SUBRESOURCE MappedResource;
        V(pd3dImmediateContext->Map(mpCBChangesEveryFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
        auto pCB = reinterpret_cast<CBChangesEveryFrame*>(MappedResource.pData);
        XMStoreFloat4x4(&pCB->mWorldViewProj, XMMatrixTranspose(mWorldViewProjection));
        XMStoreFloat4x4(&pCB->mInvProjectionView, XMMatrixTranspose(mInvProjectionView));
        XMStoreFloat4x4(&pCB->mWorld, XMMatrixTranspose(mmWorld));
        //pCB->vMeshColor = mvMeshColor;
        pd3dImmediateContext->Unmap(mpCBChangesEveryFrame, 0);
    }
    void Render(ID3D11DeviceContext* pd3dImmediateContext )
    {
        UpdateParameters(pd3dImmediateContext);
        // Render the mesh
        UINT Strides[1] = { sizeof(PlaneVertex) };
        UINT Offsets[1] = { 0 };
        ID3D11Buffer* pVB[1] = { mpVertexBuffer };
        pd3dImmediateContext->IASetVertexBuffers( 0, 1, pVB, Strides, Offsets );
        pd3dImmediateContext->IASetIndexBuffer(mpIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
        // Set the Vertex Layout
        pd3dImmediateContext->IASetInputLayout( mpVertexLayout );
        pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        pd3dImmediateContext->VSSetShader(mpVertexShader, nullptr, 0);
        pd3dImmediateContext->VSSetConstantBuffers(0, 1, &mpCBChangesEveryFrame);
        pd3dImmediateContext->PSSetShader(mpPixelShader, nullptr, 0);
        pd3dImmediateContext->PSSetConstantBuffers(0, 1, &mpCBChangesEveryFrame);
        pd3dImmediateContext->DrawIndexed( mPlane.mIndexBuffer.size(), 0, 0);
    }


    bool IntersectionTest(const CBaseCamera &renderCamera);
    void GetIntersectionRange(const CBaseCamera &renderCamera);
};


