//--------------------------------------------------------------------------------------
// File: Tutorial12.cpp
//
// Advanced Pixel Shader
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"
#include "SDKmesh.h"
#include "DDSTextureLoader.h"

#include <d3dx11effect.h>
#include "meshgenerator.h"

#pragma warning( disable : 4100 )

using namespace DirectX;

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
CModelViewerCamera                  g_Camera;               // A model viewing camera
CDXUTDialogResourceManager          g_DialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg                     g_SettingsDlg;          // Device settings dialog
CDXUTTextHelper*                    g_pTxtHelper = nullptr;
CDXUTDialog                         g_HUD;                  // manages the 3D UI
CDXUTDialog                         g_SampleUI;             // dialog for sample specific controls
CDXUTComboBox*                      g_ObjectModelSelectCombo;

XMMATRIX                            g_World;
bool                                g_bSpinning = false;
bool                                g_bWireframe = false;
bool                                g_bDrawBackground = false;
int                                 g_iRenderMode = 0;
ID3DX11Effect*                      g_pEffect = nullptr;

ID3D11InputLayout*                  g_pVertexLayout = nullptr;
ID3D11InputLayout*                  g_pSphereVertexLayout = nullptr;
ID3D11InputLayout*                  g_pBackgroundVertexLayout = nullptr;
ID3D11InputLayout*                  g_pObjModelLayout = nullptr;

ID3DX11EffectTechnique*             g_pTechnique = nullptr;
ID3DX11EffectTechnique*             g_pSimpleTechnique = nullptr;
ID3DX11EffectTechnique*             g_pPhongTechnique = nullptr;
ID3DX11EffectTechnique*             g_pBackgroundTechnique = nullptr;
ID3DX11EffectTechnique*             g_p3DScanModelTechnique = nullptr;

ID3D11ShaderResourceView*           g_pEnvMapSRV;
ID3DX11EffectShaderResourceVariable* g_ptxDiffuseVariable = nullptr;
ID3DX11EffectMatrixVariable*        g_pWorldVariable = nullptr;
ID3DX11EffectMatrixVariable*        g_pViewVariable = nullptr;
ID3DX11EffectMatrixVariable*        g_pProjectionVariable = nullptr;
ID3DX11EffectShaderResourceVariable* g_pEnvMapVariable = nullptr;
ID3DX11EffectVectorVariable*        g_vCameraPosVariable = nullptr;
ID3DX11EffectScalarVariable*        g_bSpinVariable = nullptr;
ID3DX11EffectScalarVariable*        g_fShininessVariable = nullptr;

ID3D11Buffer*						g_pVertexBuffer = nullptr;
ID3D11Buffer*						g_pBackgroundVertexBuffer = nullptr;
ID3D11Buffer*						g_pIndexBuffer = nullptr;
ID3D11Buffer*						g_pPositionBuffer = nullptr;
ID3D11Buffer*						g_pNormalBuffer = nullptr;
ID3D11Buffer*						g_pTexCoordBuffer = nullptr;

ID3D11RasterizerState*              g_pRasterizerStateWireframe = nullptr;
ID3D11RasterizerState*              g_pRasterizerStateSolid = nullptr;

//--------------------------------------------------------------------------------------
// Layout Object
//--------------------------------------------------------------------------------------

D3D11_INPUT_ELEMENT_DESC g_ObjectLayout[] =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

D3D11_INPUT_ELEMENT_DESC g_ObjModelLayout[] =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    2, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

// Define the sphere input layout
D3D11_INPUT_ELEMENT_DESC g_SimpleLayout[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------

#define IDC_TOGGLEFULLSCREEN    1
#define IDC_TOGGLEREF           2
#define IDC_CHANGEDEVICE        3
#define IDC_TOGGLEWARP          4
#define IDC_TOGGLESPIN          5
#define IDC_TOGGLEWIREFRAME     6
#define IDC_TOGGLEBACKGROUND    7

#define IDC_RENDER_MODE      (IDC_TOGGLEBACKGROUND + 1) 
#define IDC_RENDER_PHONG     (IDC_TOGGLEBACKGROUND + 2) 
#define IDC_RENDER_BLINPHONG (IDC_TOGGLEBACKGROUND + 3) 
#define IDC_RENDER_ENVMAP	 (IDC_TOGGLEBACKGROUND + 4) 
#define IDC_RENDER_SIMPLE	 (IDC_TOGGLEBACKGROUND + 5) 
#define IDC_SELECTED_OBJECT  (IDC_TOGGLEBACKGROUND + 6)
#define IDC_MODEL_THETA_STATIC (IDC_TOGGLEBACKGROUND + 7)
#define IDC_MODEL_THETA        (IDC_TOGGLEBACKGROUND + 8)
#define IDC_MODEL_PHI_STATIC   (IDC_TOGGLEBACKGROUND + 9)
#define IDC_MODEL_PHI          (IDC_TOGGLEBACKGROUND + 10)

UINT     g_uNumIndex;
UINT     g_uNumVertex;
UINT     g_uVertexStride;
MeshType g_eMeshType;

float g_fTheta = PI;
float g_fPhi   = PI * 2.0;


enum RENDER_MODE
{
	PHONG_RENDER,
	BLINPHONG_RENDER,
	ENVMAP_RENDER,
	SIMPLE_RENDER,
};

enum OBJECTMODEL
{
	SphereModel,
	CylinderModel,
	ConeModel,
	TorusModel,
    PlaneModel,
	ThreeDScanModel,
};

OBJECTMODEL g_eObjectModel = SphereModel;
//OBJECTMODEL g_eObjectModel = PlaneModel;
//Obj g_3DscanModel;

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
void RenderText();
void InitApp();

void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime, float fElapsedTime, void* pUserContext);
HRESULT CreateMeshBuffers(ID3D11Device* pd3dDevice,MeshGenerator &polyMesh);
HRESULT CreateSeperateBuffers(ID3D11Device* pd3dDevice, ThreeDScanObject &objModel);


//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    return true;
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    return true;
}



//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                      void* pUserContext )
{
    HRESULT hr = S_OK;

    auto pd3dImmediateContext = DXUTGetD3D11DeviceContext();
    V_RETURN( g_DialogResourceManager.OnD3D11CreateDevice( pd3dDevice, pd3dImmediateContext ) );
    V_RETURN( g_SettingsDlg.OnD3D11CreateDevice( pd3dDevice ) );
    g_pTxtHelper = new CDXUTTextHelper( pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15 );

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

#if D3D_COMPILER_VERSION >= 46

    // Read the D3DX effect file
    WCHAR str[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"EnvMap.fx" ) );
    V_RETURN( D3DX11CompileEffectFromFile( str, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, dwShaderFlags, 0, pd3dDevice, &g_pEffect, nullptr) );

#else

    ID3DBlob* pEffectBuffer = nullptr;
    V_RETURN( DXUTCompileFromFile( L"EnvMap.fx", nullptr, "none", "fx_5_0", dwShaderFlags, 0, &pEffectBuffer ) );
    hr = D3DX11CreateEffectFromMemory( pEffectBuffer->GetBufferPointer(), pEffectBuffer->GetBufferSize(), 0, pd3dDevice, &g_pEffect );
    SAFE_RELEASE( pEffectBuffer );
    if ( FAILED(hr) )
        return hr;
#endif

    // Obtain the technique
    g_pTechnique              = g_pEffect->GetTechniqueByName( "EnvMapRender" );
    g_pSimpleTechnique        = g_pEffect->GetTechniqueByName( "SimpleRender" );
    g_pPhongTechnique         = g_pEffect->GetTechniqueByName( "PhongRender" );
    g_pBackgroundTechnique    = g_pEffect->GetTechniqueByName( "BackgroundRender" );
    g_p3DScanModelTechnique   = g_pEffect->GetTechniqueByName( "ThreeDScanModelRender" );

    // Obtain the variables
    g_pWorldVariable      = g_pEffect->GetVariableByName( "World" )->AsMatrix();
    g_pViewVariable       = g_pEffect->GetVariableByName( "View" )->AsMatrix();
    g_pProjectionVariable = g_pEffect->GetVariableByName( "Projection" )->AsMatrix();
    g_pEnvMapVariable    = g_pEffect->GetVariableByName( "g_txEnvMap" )->AsShaderResource();
    g_ptxDiffuseVariable = g_pEffect->GetVariableByName( "g_txDiffuse" )->AsShaderResource();
    g_vCameraPosVariable  = g_pEffect->GetVariableByName( "CameraPosWorld" )->AsVector();
    g_bSpinVariable       = g_pEffect->GetVariableByName( "SpinBackground" )->AsScalar();
    g_fShininessVariable  = g_pEffect->GetVariableByName( "Shininess" )->AsScalar();

    // Create the input layout
    D3DX11_PASS_DESC PassDesc;
    UINT numElements = ARRAYSIZE(g_ObjectLayout);
    V_RETURN( g_pTechnique->GetPassByIndex( 0 )->GetDesc( &PassDesc ) );
    V_RETURN( pd3dDevice->CreateInputLayout(g_ObjectLayout, numElements, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &g_pVertexLayout ) );

	numElements = ARRAYSIZE(g_SimpleLayout);
	V_RETURN(g_pSimpleTechnique->GetPassByIndex(0)->GetDesc(&PassDesc));
	V_RETURN(pd3dDevice->CreateInputLayout(g_SimpleLayout, numElements, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &g_pSphereVertexLayout));

	numElements = ARRAYSIZE(g_SimpleLayout);
	V_RETURN(g_pBackgroundTechnique->GetPassByIndex(0)->GetDesc(&PassDesc));
	V_RETURN(pd3dDevice->CreateInputLayout(g_SimpleLayout, numElements, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &g_pBackgroundVertexLayout));

	numElements = ARRAYSIZE(g_ObjModelLayout);
    V_RETURN(g_p3DScanModelTechnique->GetPassByIndex(0)->GetDesc(&PassDesc));
	V_RETURN(pd3dDevice->CreateInputLayout(g_ObjModelLayout, numElements, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &g_pObjModelLayout));


	// Create solid and wireframe rasterizer state objects
	D3D11_RASTERIZER_DESC RasterDesc;
	ZeroMemory(&RasterDesc, sizeof(D3D11_RASTERIZER_DESC));
	RasterDesc.CullMode = D3D11_CULL_NONE;
	RasterDesc.DepthClipEnable = TRUE;
	RasterDesc.FillMode = D3D11_FILL_WIREFRAME;
	pd3dDevice->CreateRasterizerState(&RasterDesc, &g_pRasterizerStateWireframe);
	RasterDesc.FillMode = D3D11_FILL_SOLID;
	pd3dDevice->CreateRasterizerState(&RasterDesc, &g_pRasterizerStateSolid);

    // Initialize the world matrices
    g_World = XMMatrixIdentity();

    // Load the Environment Map
    V_RETURN( DXUTCreateShaderResourceViewFromFile( pd3dDevice,L"uffizi_cross32.dds", &g_pEnvMapSRV ) );
    g_pEnvMapVariable->SetResource( g_pEnvMapSRV );

    // Setup the camera's view parameters
    static const XMVECTORF32 s_Eye = { 0.0f, 3.0f, -10.0f, 0.f };
    static const XMVECTORF32 s_At  = { 0.0f, 0.0f, 0.0f, 0.f };
    g_Camera.SetViewParams( s_Eye, s_At );
    //CreateSeperateBuffers(pd3dDevice, g_3DscanModel);
    return S_OK;
}

HRESULT CreateSeperateBuffers(ID3D11Device* pd3dDevice, ThreeDScanObject &objModel)
{
	HRESULT  hr;
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage          = D3D11_USAGE_DEFAULT;
	bd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));

    bd.ByteWidth     = objModel.mSizePositionBuffer;
    InitData.pSysMem = objModel.mPostionBuffer;
    SAFE_RELEASE( g_pPositionBuffer);
	V_RETURN(pd3dDevice->CreateBuffer(&bd, &InitData, &g_pPositionBuffer));

    bd.ByteWidth     = objModel.mSizeNormalBuffer;
    InitData.pSysMem = objModel.mNormalBuffer;
    SAFE_RELEASE( g_pNormalBuffer);
	V_RETURN(pd3dDevice->CreateBuffer(&bd, &InitData, &g_pNormalBuffer));

	bd.ByteWidth = objModel.mSizeIndexBuffer;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = &(objModel.mIndexBuffer[0]);
    SAFE_RELEASE( g_pIndexBuffer);
	V_RETURN(pd3dDevice->CreateBuffer(&bd, &InitData, &g_pIndexBuffer));

	return S_OK;
}

HRESULT CreateMeshBuffers(ID3D11Device* pd3dDevice, MeshGenerator &polyMesh)
{
	HRESULT  hr;
	g_uNumIndex     = polyMesh.mNumIndex;
	g_uNumVertex    = polyMesh.mNumVertex;
	g_uVertexStride = polyMesh.mVertexStride;
	g_eMeshType     = polyMesh.mType;

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.ByteWidth = polyMesh.mVertexStride * polyMesh.mNumVertex;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = polyMesh.mVertexBuffer;
    SAFE_RELEASE( g_pVertexBuffer);
	V_RETURN(pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer));

	bd.ByteWidth = sizeof(int) * polyMesh.mNumIndex;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = &(polyMesh.mIndexBuffer[0]);
    SAFE_RELEASE( g_pIndexBuffer);
	V_RETURN(pd3dDevice->CreateBuffer(&bd, &InitData, &g_pIndexBuffer));

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
    V_RETURN( g_SettingsDlg.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

    // Setup the camera's projection parameters
    float fAspectRatio = pBackBufferSurfaceDesc->Width / ( FLOAT )pBackBufferSurfaceDesc->Height;
    g_Camera.SetProjParams( XM_PI / 4, fAspectRatio, 0.1f, 50000.0f );
    g_Camera.SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );
    g_Camera.SetButtonMasks( MOUSE_LEFT_BUTTON, MOUSE_WHEEL, MOUSE_MIDDLE_BUTTON );

    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
    g_HUD.SetSize( 170, 170 );
    g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height - 400 );
    g_SampleUI.SetSize( 170, 170 );

    // background vertex buffer.
    // Map texels to pixels 
    XMFLOAT4 bgVertex[4];
    float fHighW = -1.0f - (1.0f / (float)pBackBufferSurfaceDesc->Width);
    float fHighH = -1.0f - (1.0f / (float)pBackBufferSurfaceDesc->Height);
    float fLowW = 1.0f + (1.0f / (float)pBackBufferSurfaceDesc->Width);
    float fLowH = 1.0f + (1.0f / (float)pBackBufferSurfaceDesc->Height);

    bgVertex[0]= XMFLOAT4(fLowW, fLowH, 1.0f, 1.0f);
    bgVertex[1]= XMFLOAT4(fLowW, fHighH, 1.0f, 1.0f);
    bgVertex[2]= XMFLOAT4(fHighW, fLowH, 1.0f, 1.0f);
    bgVertex[3]= XMFLOAT4(fHighW, fHighH, 1.0f, 1.0f);
#if 0
    bgVertex[0] = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    bgVertex[1] = XMFLOAT4(1.0f, -1.0f, 1.0f, 1.0f);
    bgVertex[2] = XMFLOAT4(-1.0f, 1.0f, 1.0f, 1.0f);
    bgVertex[3] = XMFLOAT4(-1.0f, -1.0f, 1.0f, 1.0f);
#endif

    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.ByteWidth = sizeof(XMFLOAT4) * 4;

	D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = bgVertex;

    SAFE_RELEASE( g_pBackgroundVertexBuffer);
    hr = pd3dDevice->CreateBuffer(&bd, &InitData, &g_pBackgroundVertexBuffer);
    if (FAILED(hr)) return hr;

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    // Update the camera's position based on user input 
    g_Camera.FrameMove( fElapsedTime );

#if 0
	XMMATRIX mRot;
    if( g_bSpinning )
    {
        mRot = XMMatrixRotationY( 10.0f * XMConvertToRadians((float)fTime) );
    }
    else
    {
        mRot = XMMatrixRotationY( XMConvertToRadians( 180.f ) );
    }

    g_World = mRot * g_World;
#endif
}


//--------------------------------------------------------------------------------------
// Render the help and statistics text
//--------------------------------------------------------------------------------------
void RenderText()
{
    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 2, 0 );
    g_pTxtHelper->SetForegroundColor( Colors::Yellow );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );
    g_pTxtHelper->End();
}

void BackgroundRender(ID3D11DeviceContext* pd3dImmediateContext)
{
	UINT stride = 0;
	UINT offset = 0;
	D3DX11_TECHNIQUE_DESC techDesc;
	HRESULT hr;

    // Render Background 
    stride = sizeof(XMFLOAT4); offset = 0;
    pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_pBackgroundVertexBuffer, &stride, &offset);
    pd3dImmediateContext->RSSetState(g_pRasterizerStateSolid);
    pd3dImmediateContext->IASetInputLayout(g_pBackgroundVertexLayout);
    V(g_pBackgroundTechnique->GetDesc(&techDesc));
    for (UINT p = 0; p < techDesc.Passes; ++p)
    {
        pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        g_pBackgroundTechnique->GetPassByIndex(p)->Apply(0, pd3dImmediateContext);
        pd3dImmediateContext->Draw(4, 0);
    }
}

void SimpleRender(ID3D11DeviceContext* pd3dImmediateContext)
{
	UINT stride = 0;
	UINT offset = 0;
	D3DX11_TECHNIQUE_DESC techDesc;
	HRESULT hr;

	if (g_bWireframe) pd3dImmediateContext->RSSetState(g_pRasterizerStateWireframe);
	else              pd3dImmediateContext->RSSetState(g_pRasterizerStateSolid);

	// Render  Object
    stride = g_uVertexStride;  offset = 0;
	pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	pd3dImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    pd3dImmediateContext->IASetInputLayout(g_pSphereVertexLayout);
    V(g_pSimpleTechnique->GetDesc(&techDesc));
    for (UINT p = 0; p < techDesc.Passes; ++p)
    {
        if (g_eMeshType == LINE_LIST)
            pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        else if (g_eMeshType == TRIANGLE_LIST)
            pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        g_pSimpleTechnique->GetPassByIndex(p)->Apply(0, pd3dImmediateContext);
        pd3dImmediateContext->DrawIndexed(g_uNumIndex, 0, 0);
    }
}

void EnvironmentMapRender(ID3D11DeviceContext* pd3dImmediateContext)
{
	UINT stride = 0;
	UINT offset = 0;
	D3DX11_TECHNIQUE_DESC techDesc;
	HRESULT hr;

	if (g_bWireframe) pd3dImmediateContext->RSSetState(g_pRasterizerStateWireframe);
	else              pd3dImmediateContext->RSSetState(g_pRasterizerStateSolid);

	// Render  Object
	stride = g_uVertexStride; offset = 0;
	pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	pd3dImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    pd3dImmediateContext->IASetInputLayout(g_pVertexLayout);
    V(g_pTechnique->GetDesc(&techDesc));
    for (UINT p = 0; p < techDesc.Passes; ++p)
    {
        if (g_eMeshType == LINE_LIST)
            pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        else if (g_eMeshType == TRIANGLE_LIST)
            pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        g_pTechnique->GetPassByIndex(p)->Apply(0, pd3dImmediateContext);
        pd3dImmediateContext->DrawIndexed(g_uNumIndex, 0, 0);
    }
}

void ObjModelRender(ID3D11DeviceContext* pd3dImmediateContext)
{
	D3DX11_TECHNIQUE_DESC techDesc;
	HRESULT hr;
	ID3D11Buffer*  buffers[2] = { g_pPositionBuffer, g_pNormalBuffer};
	UINT stride[2] = { 3 * sizeof(float), 3 * sizeof(float) };
	UINT offset[2] = { 0 };

	if (g_bWireframe) pd3dImmediateContext->RSSetState(g_pRasterizerStateWireframe);
	else              pd3dImmediateContext->RSSetState(g_pRasterizerStateSolid);

	// Render  Object
	pd3dImmediateContext->IASetVertexBuffers(0, 1, buffers, stride, offset);
	pd3dImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    pd3dImmediateContext->IASetInputLayout(g_pObjModelLayout);
    V(g_pPhongTechnique->GetDesc(&techDesc));
    for (UINT p = 0; p < techDesc.Passes; ++p)
    {
        if (g_eMeshType == LINE_LIST)
            pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        else if (g_eMeshType == TRIANGLE_LIST)
            pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        g_pPhongTechnique->GetPassByIndex(p)->Apply(0, pd3dImmediateContext);
        pd3dImmediateContext->DrawIndexed(g_uNumIndex, 0, 0);
    }
}

void RenderThreeDScanModel(ID3D11DeviceContext* pd3dImmediateContext)
{
	UINT stride = 0;
	UINT offset = 0;
	D3DX11_TECHNIQUE_DESC techDesc;
	HRESULT hr;

	if (g_bWireframe) pd3dImmediateContext->RSSetState(g_pRasterizerStateWireframe);
	else              pd3dImmediateContext->RSSetState(g_pRasterizerStateSolid);

	// Render  Object
	stride = g_uVertexStride; offset = 0;
	pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	pd3dImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    pd3dImmediateContext->IASetInputLayout(g_pVertexLayout);
    V(g_p3DScanModelTechnique->GetDesc(&techDesc));
    for (UINT p = 0; p < techDesc.Passes; ++p)
    {
        if (g_eMeshType == LINE_LIST)
            pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        else if (g_eMeshType == TRIANGLE_LIST)
            pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        g_p3DScanModelTechnique->GetPassByIndex(p)->Apply(0, pd3dImmediateContext);
        pd3dImmediateContext->DrawIndexed(g_uNumIndex, 0, 0);
    }
}

void PhongRender(ID3D11DeviceContext* pd3dImmediateContext)
{
	UINT stride = 0;
	UINT offset = 0;
	D3DX11_TECHNIQUE_DESC techDesc;
	HRESULT hr;

	if (g_bWireframe) pd3dImmediateContext->RSSetState(g_pRasterizerStateWireframe);
	else              pd3dImmediateContext->RSSetState(g_pRasterizerStateSolid);

	// Render  Object
	stride = g_uVertexStride; offset = 0;
	pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	pd3dImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    pd3dImmediateContext->IASetInputLayout(g_pVertexLayout);
    V(g_pPhongTechnique->GetDesc(&techDesc));
    for (UINT p = 0; p < techDesc.Passes; ++p)
    {
        if (g_eMeshType == LINE_LIST)
            pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        else if (g_eMeshType == TRIANGLE_LIST)
            pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        g_pPhongTechnique->GetPassByIndex(p)->Apply(0, pd3dImmediateContext);
        pd3dImmediateContext->DrawIndexed(g_uNumIndex, 0, 0);
    }
}

void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime, float fElapsedTime, void* pUserContext)
{
	// If the settings dialog is being shown, then render it instead of rendering the app's scene
	if (g_SettingsDlg.IsActive())
	{
		g_SettingsDlg.OnRender(fElapsedTime);
		return;
	}
	switch (g_eObjectModel)
	{
	case SphereModel:
        CreateMeshBuffers(pd3dDevice, Sphere(16, 32, 1.0, 0.0, g_fTheta, g_fPhi));
		break;
	case CylinderModel:
        CreateMeshBuffers(pd3dDevice, Cylinder(16, 32, -1.0, 1.0, 1.0, g_fPhi));
		break;
	case ConeModel:
        CreateMeshBuffers(pd3dDevice, Cone(16, 32, -1.0, 1.0, 1.0, g_fPhi));
		break;
	case TorusModel:
        CreateMeshBuffers(pd3dDevice, Torus(32, 64, 0.25, 1.0, 2.0 * g_fTheta, g_fPhi));
		break;
	case PlaneModel:
        CreateMeshBuffers(pd3dDevice, PlaneMesh());
		break;
	case ThreeDScanModel:
       // CreateSeperateBuffers(pd3dDevice, g_3DscanModel);
		break;
	}

	// Clear the back buffer & depth stencil
	XMVECTORF32 background = { 0.1, 0.1, 0.1, 1.000000000f };
	auto pRTV = DXUTGetD3D11RenderTargetView();
	pd3dImmediateContext->ClearRenderTargetView(pRTV, background);
	auto pDSV = DXUTGetD3D11DepthStencilView();
	pd3dImmediateContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH, 1.0, 0);

	XMMATRIX mView = g_Camera.GetViewMatrix();
	XMMATRIX mProj = g_Camera.GetProjMatrix();
    XMVECTORF32 vCameraPos;
    vCameraPos.v = g_Camera.GetEyePt();
	g_vCameraPosVariable->SetFloatVector(vCameraPos.f);
	XMMATRIX mWorldViewProjection = g_World * mView * mProj;
	g_World = g_Camera.GetWorldMatrix(); // update world matrix from model camera
	g_pWorldVariable->SetMatrix((float*)&g_World);
	g_pViewVariable->SetMatrix((float*)&mView);
	g_pProjectionVariable->SetMatrix((float*)&mProj);
	g_bSpinVariable->SetBool(g_bSpinning);
    g_fShininessVariable->SetFloat(100);

    // Render Background 
    if (g_bDrawBackground)
    {
        BackgroundRender(pd3dImmediateContext);
    }

    if (g_eObjectModel == ThreeDScanModel)
    {
        RenderThreeDScanModel(pd3dImmediateContext);
    }
    else
    {
        switch (g_iRenderMode)
        {
        case PHONG_RENDER:
            PhongRender(pd3dImmediateContext);
            break;
        case BLINPHONG_RENDER:
            break;
        case ENVMAP_RENDER:
            EnvironmentMapRender(pd3dImmediateContext);
            break;
        case SIMPLE_RENDER:
            SimpleRender(pd3dImmediateContext);
            break;
        default:
            break;

        }
    }

	g_HUD.OnRender(fElapsedTime);
	g_SampleUI.OnRender(fElapsedTime);
	RenderText();
}

//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11ReleasingSwapChain();
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11DestroyDevice();
    g_SettingsDlg.OnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();
    SAFE_DELETE( g_pTxtHelper );

    SAFE_RELEASE( g_pVertexLayout );
    SAFE_RELEASE( g_pSphereVertexLayout );
    SAFE_RELEASE( g_pBackgroundVertexLayout );
    SAFE_RELEASE( g_pEffect );
    SAFE_RELEASE( g_pEnvMapSRV );
    SAFE_RELEASE( g_pRasterizerStateWireframe);
    SAFE_RELEASE( g_pRasterizerStateSolid);
    SAFE_RELEASE( g_pVertexBuffer);
    SAFE_RELEASE( g_pIndexBuffer);
    SAFE_RELEASE( g_pBackgroundVertexBuffer);
    SAFE_RELEASE( g_pObjModelLayout );
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                          bool* pbNoFurtherProcessing, void* pUserContext )
{
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass messages to settings dialog if its active
    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
    *pbNoFurtherProcessing = g_SampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass all remaining windows messages to camera so it can respond to user input
    g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
    if( bKeyDown )
    {
		static XMFLOAT3 translate; 
        switch( nChar )
        {
            case  VK_LEFT: // Change as needed  
				translate.x += 1;
                break;
            case VK_RIGHT: // Change as needed  
				translate.x -= 1;
                break;
            case VK_UP: // Change as needed  
				translate.y -= 1;
                break;
            case VK_DOWN: // Change as needed  
				translate.y += 1;
                break;
        }
		g_Camera.SetModelCenter(translate);
    }
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    switch( nControlID )
    {
        case IDC_TOGGLEFULLSCREEN:
            DXUTToggleFullScreen();
            break;
        case IDC_TOGGLEREF:
            DXUTToggleREF();
            break;
        case IDC_CHANGEDEVICE:
            g_SettingsDlg.SetActive( !g_SettingsDlg.IsActive() );
            break;
        case IDC_TOGGLEWARP:
            DXUTToggleWARP();
            break;
        case IDC_TOGGLEWIREFRAME:
        {
            g_bWireframe = g_SampleUI.GetCheckBox( IDC_TOGGLEWIREFRAME )->GetChecked();
            break;
        }
        case IDC_TOGGLESPIN:
        {
            g_bSpinning = g_SampleUI.GetCheckBox( IDC_TOGGLESPIN )->GetChecked();
            break;
        }
        case IDC_TOGGLEBACKGROUND:
        {
            g_bDrawBackground = g_SampleUI.GetCheckBox( IDC_TOGGLEBACKGROUND )->GetChecked();
            break;
        }
        case IDC_RENDER_PHONG:
        {
            g_iRenderMode = PHONG_RENDER;
            break;
        }
        case IDC_RENDER_BLINPHONG:
        {
            g_iRenderMode = BLINPHONG_RENDER;
            break;
        }
        case IDC_RENDER_ENVMAP:
        {
            g_iRenderMode = ENVMAP_RENDER;
            break;
        }
        case IDC_RENDER_SIMPLE:
        {
            g_iRenderMode = SIMPLE_RENDER;
            break;
        }
		case IDC_SELECTED_OBJECT:
		{
			g_eObjectModel = (OBJECTMODEL)PtrToUlong(g_ObjectModelSelectCombo->GetSelectedData());
		}
        case IDC_MODEL_PHI:
        {
            g_fPhi = g_SampleUI.GetSlider(IDC_MODEL_PHI)->GetValue() / 100.0f;
            g_fPhi *= (2.0 * PI);

            WCHAR sz[100];
            swprintf_s(sz, L"Model  Phi: %2.5f", g_fPhi);
            g_SampleUI.GetStatic(IDC_MODEL_PHI_STATIC)->SetText(sz);
        }
        case IDC_MODEL_THETA:
        {
            g_fTheta = g_SampleUI.GetSlider(IDC_MODEL_THETA)->GetValue() / 100.0f;
            g_fTheta *= PI;

            WCHAR sz[100];
            swprintf_s(sz, L"Model Theta: %2.5f", g_fTheta);
            g_SampleUI.GetStatic(IDC_MODEL_THETA_STATIC)->SetText(sz);
        }
    }
}


//--------------------------------------------------------------------------------------
// Call if device was removed.  Return true to find a new device, false to quit
//--------------------------------------------------------------------------------------
bool CALLBACK OnDeviceRemoved( void* pUserContext )
{
    return true;
}


//--------------------------------------------------------------------------------------
// Initialize everything and go into a render loop
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#ifdef _DEBUG
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // DXUT will create and use the best device
    // that is available on the system depending on which D3D callbacks are set below

    // Set general DXUT callbacks
    DXUTSetCallbackFrameMove( OnFrameMove );
    DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackDeviceRemoved( OnDeviceRemoved );

    // Set the D3D11 DXUT callbacks. Remove these sets if the app doesn't need to support D3D11
    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );

    // Perform any application-level initialization here

    DXUTInit( true, true, nullptr ); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen

    InitApp();
    DXUTCreateWindow( L"EnvMap&LightingModel" );

    // Only require 10-level hardware or later
    DXUTCreateDevice( D3D_FEATURE_LEVEL_10_0, true, 800, 640 );
    DXUTMainLoop(); // Enter into the DXUT render loop

    // Perform any application-level cleanup here

    return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
    g_bSpinning = true;

    g_SettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );
    g_SampleUI.Init( &g_DialogResourceManager );

    g_HUD.SetCallback( OnGUIEvent ); int iY = 10;
    g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 0, iY, 170, 22 );
    g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 0, iY += 26, 170, 22, VK_F2 );
    g_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 0, iY += 26, 170, 22, VK_F3 );
    g_HUD.AddButton( IDC_TOGGLEWARP, L"Toggle WARP (F4)", 0, iY += 26, 170, 22, VK_F4 );

    g_SampleUI.SetCallback( OnGUIEvent );
	iY = 0;
	g_SampleUI.AddComboBox(IDC_SELECTED_OBJECT, 0, iY += 5, 170, 22, VK_F8, false, &g_ObjectModelSelectCombo);
	g_ObjectModelSelectCombo->AddItem(L"Sphere Model",ULongToPtr(SphereModel));
	g_ObjectModelSelectCombo->AddItem(L"Cylinder Model",ULongToPtr(CylinderModel));
	g_ObjectModelSelectCombo->AddItem(L"Cone Model", ULongToPtr(ConeModel));
	g_ObjectModelSelectCombo->AddItem(L"Torus Model", ULongToPtr(TorusModel));
	g_ObjectModelSelectCombo->AddItem(L"Plane Model", ULongToPtr(PlaneModel));
	//g_ObjectModelSelectCombo->AddItem(L"Obj Model", ULongToPtr(ObjModel));


    WCHAR sz[100];
    swprintf_s(sz, L"Model Theta: %2.5f", g_fTheta);
    g_SampleUI.AddStatic(IDC_MODEL_THETA_STATIC, sz, 0, iY += 20, 170, 6);
    g_SampleUI.AddSlider(IDC_MODEL_THETA, 0, iY += 20, 170, 10, 0, 100, 100);

    swprintf_s(sz, L"Model   Phi: %2.5f", g_fPhi);
    g_SampleUI.AddStatic(IDC_MODEL_PHI_STATIC, sz, 0, iY += 10, 170, 6);
    g_SampleUI.AddSlider(IDC_MODEL_PHI, 0, iY += 20, 170, 10, 0, 100, 100);

    g_SampleUI.AddCheckBox( IDC_TOGGLEWIREFRAME,  L"Wireframe Mode", 0, iY += 26, 170, 22, g_bWireframe );
    g_SampleUI.AddCheckBox( IDC_TOGGLEBACKGROUND, L"Draw Background", 0, iY += 26, 170, 22, g_bDrawBackground );
    g_SampleUI.AddCheckBox( IDC_TOGGLESPIN,       L"Rotate Background", 0, iY += 26, 170, 22, g_bSpinning );

	iY += 5;
	g_SampleUI.AddRadioButton(IDC_RENDER_PHONG,     IDC_RENDER_MODE, L"Phong Render", 0, iY += 26, 170, 22);
	g_SampleUI.AddRadioButton(IDC_RENDER_BLINPHONG, IDC_RENDER_MODE, L"Bllin-Phong Render", 0, iY += 26, 170, 22);
	g_SampleUI.AddRadioButton(IDC_RENDER_ENVMAP,    IDC_RENDER_MODE, L"Env-Map Render", 0, iY += 26, 170, 22);
	g_SampleUI.AddRadioButton(IDC_RENDER_SIMPLE,    IDC_RENDER_MODE, L"Simple Render", 0, iY += 26, 170, 22);
	g_SampleUI.GetRadioButton(IDC_RENDER_PHONG)->SetChecked(true);

}

 