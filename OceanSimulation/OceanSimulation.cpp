#include "DXUT.h"
#include "SDKmisc.h"
#include "DXUTgui.h"
#include "DXUTcamera.h"

#include "OceanSurface.h"
#include "Camera.h"

#pragma warning( disable : 4100 )

using namespace DirectX;

OceanSurface oceansurface;

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
CModelViewerCamera                  g_Camera;               // A model viewing camera
CModelViewerCamera                  g_ObserveCamera;
Camera  g_renderCamera(XMFLOAT3(0, 10, -10), XMFLOAT3(0, 0, 0));

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
ID3D11SamplerState*         g_pSamplerLinear = nullptr;
XMMATRIX                    g_World;
XMFLOAT4                    g_vMeshColor( 0.7f, 0.7f, 0.7f, 1.0f );

bool                                g_bWireframe = true;
float                               g_fModelWaviness = 0.0f;

ID3D11RasterizerState*              g_pRasterizerStateWireframe = nullptr;
ID3D11RasterizerState*              g_pRasterizerStateSolid = nullptr;

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN    1
#define IDC_TOGGLEREF           2
#define IDC_CHANGEDEVICE        3
#define IDC_TOGGLEWARP          4
#define IDC_TOGGLESPIN          5
#define IDC_PUFF_SCALE          6
#define IDC_PUFF_STATIC         7
#define IDC_TOGGLEWIREFRAME     8
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

    // Create the sample state
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory( &sampDesc, sizeof(sampDesc) );
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    V_RETURN( pd3dDevice->CreateSamplerState( &sampDesc, &g_pSamplerLinear ) );

    // Create solid and wireframe rasterizer state objects
    D3D11_RASTERIZER_DESC RasterDesc;
    ZeroMemory(&RasterDesc, sizeof(D3D11_RASTERIZER_DESC));
    RasterDesc.CullMode = D3D11_CULL_NONE;
    RasterDesc.DepthClipEnable = TRUE;
    RasterDesc.FillMode = D3D11_FILL_WIREFRAME;
    pd3dDevice->CreateRasterizerState(&RasterDesc, &g_pRasterizerStateWireframe);
    RasterDesc.FillMode = D3D11_FILL_SOLID;
    pd3dDevice->CreateRasterizerState(&RasterDesc, &g_pRasterizerStateSolid);

    // Initialize the view matrix
    static const XMVECTORF32 s_Eye = { 0.0f, 10.f, -10.0f, 0.f };
    static const XMVECTORF32 s_At = { 0.0f, 0.0f, 0.0f, 0.f };
    static const XMVECTORF32 s_Up = { 0.0f, 1.0f, 0.0f, 0.f };
    //g_View = XMMatrixLookAtLH( s_Eye, s_At, s_Up );
    g_Camera.SetViewParams(s_Eye, s_At);

    const XMVECTORF32 Eye = { 400.0f, 0.f, 0.0f, 0.f };
    const XMVECTORF32 At = { 0.0f, 1.0f, 0.0f, 0.f };
    g_ObserveCamera.SetViewParams(Eye, At);

    
    oceansurface.CreateConstBuffer(pd3dDevice);
    oceansurface.CreateEffects(pd3dDevice, pUserContext);
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    // Setup the projection parameters
    float fAspect = static_cast<float>( pBackBufferSurfaceDesc->Width ) / static_cast<float>( pBackBufferSurfaceDesc->Height );
    g_renderCamera.UpdateAspect(fAspect);
    oceansurface.setWindowAspect(fAspect);
   // oceansurface.setSize(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    // Update the camera's position based on user input 
    g_Camera.FrameMove(fElapsedTime);

    // Rotate cube around the origin
    //g_World = XMMatrixRotationY( 60.0f * XMConvertToRadians((float)fTime) );

    // Modify the color
    g_vMeshColor.x = ( sinf( ( float )fTime * 1.0f ) + 1.0f ) * 0.5f;
    g_vMeshColor.y = ( cosf( ( float )fTime * 3.0f ) + 1.0f ) * 0.5f;
    g_vMeshColor.z = ( sinf( ( float )fTime * 5.0f ) + 1.0f ) * 0.5f;
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
                                  double fTime, float fElapsedTime, void* pUserContext )
{
    // Clear the back buffer
    // Clear the depth stencil
    auto pRTV = DXUTGetD3D11RenderTargetView();
    pd3dImmediateContext->ClearRenderTargetView( pRTV, Colors::MidnightBlue );
    auto pDSV = DXUTGetD3D11DepthStencilView();
    pd3dImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );

    // set Rasterizer state
    pd3dImmediateContext->RSSetState(g_pRasterizerStateWireframe);

    oceansurface.Render(pd3dDevice, pd3dImmediateContext, g_renderCamera);
    oceansurface.ObserveRenderCameraFrustum(pd3dDevice, pd3dImmediateContext, g_renderCamera);
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
    SAFE_RELEASE( g_pSamplerLinear );
    SAFE_RELEASE( g_pRasterizerStateWireframe );
    SAFE_RELEASE( g_pRasterizerStateSolid );
    oceansurface.Destroy();
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                          bool* pbNoFurtherProcessing, void* pUserContext )
{
    // Pass all remaining windows messages to camera so it can respond to user input
    g_renderCamera.HandleMessages(hWnd, uMsg, wParam, lParam);
    return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
    if( bKeyDown )
    {
        switch( nChar )
        {
            case VK_F1:
            {
                int x = oceansurface.mXSize;
                int y = oceansurface.mYSize;
                x = x + 4 > 64 ? 64 : x + 4;
                y = x;
                oceansurface.mXSize = x;
                oceansurface.mYSize = y;
                break;
            }
            case VK_F2:
            {
                int x = oceansurface.mXSize;
                int y = oceansurface.mYSize;
                x = x - 4 < 4 ? 4 : x - 4;
                y = x;
                oceansurface.mXSize = x;
                oceansurface.mYSize = y;
                break;
            }
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
    DXUTCreateWindow( L"Ocean Simulation" );

    // Only require 10-level hardware or later
    DXUTCreateDevice( D3D_FEATURE_LEVEL_10_0, true, 800, 600 );
    DXUTMainLoop(); // Enter into the DXUT render loop

    // Perform any application-level cleanup here

    return DXUTGetExitCode();
}
