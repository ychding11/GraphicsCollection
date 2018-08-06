#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"
#include "resource.h"
#include "BezierSurface.h"
#include "IDataSource.h"
#include "ShaderContainer.h"
#include "CameraLayer.h"

#pragma warning( disable : 4100 )

using namespace DirectX;

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CDXUTDialogResourceManager          g_DialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg                     g_D3DSettingsDlg;        // Device settings dialog
CDXUTDialog                         g_HUD;                   // manages the 3D   
CDXUTDialog                         g_SampleUI;              // dialog for sample specific controls
CDXUTTextHelper*                    g_pTxtHelper = nullptr;

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN      1
#define IDC_TOGGLEREF             3
#define IDC_CHANGEDEVICE          4

#define IDC_PATCH_SUBDIVS         5
#define IDC_PATCH_SUBDIVS_STATIC  6
#define IDC_TOGGLE_LINES          7
#define IDC_PARTITION_MODE        8
#define IDC_PARTITION_INTEGER     9
#define IDC_PARTITION_FRAC_EVEN   10
#define IDC_PARTITION_FRAC_ODD    11
#define IDC_SINGLE_PATCH          12
#define IDC_TRI_DOMAIN            13
#define IDC_SELECTED_OBJECT       14

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext );

bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo, DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext );
void CALLBACK OnD3D11DestroyDevice( void* pUserContext );
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime, float fElapsedTime, void* pUserContext );
void CALLBACK KeyboardProc(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext);

void InitApp();
void RenderText();

int WINAPI wWinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // DXUT will create and use the best device that is available on the system depending on which D3D callbacks are set below

    // Set DXUT callbacks
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackMsgProc( MsgProc );
	DXUTSetCallbackKeyboard(KeyboardProc);
    DXUTSetCallbackFrameMove( OnFrameMove );

    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );

    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );

    InitApp();
    DXUTInit( true, true ); // Parse the command line, show msgboxes on error, and an extra cmd line param to force REF for now
    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"Tessellation of Bezier Surface" );
    DXUTCreateDevice( D3D_FEATURE_LEVEL_11_0,  true, 1920, 1080 );
    DXUTMainLoop();

    return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
    // Initialize dialogs
    g_D3DSettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );
    g_SampleUI.Init( &g_DialogResourceManager );

    g_HUD.SetCallback( OnGUIEvent ); int iY = 20;
    g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 0, iY, 170, 22 );
    g_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 0, iY += 26, 170, 22, VK_F3 );
    g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 0, iY += 26, 170, 22, VK_F2 );

    g_SampleUI.SetCallback( OnGUIEvent ); iY = 0;

#if 0
    g_SampleUI.AddComboBox(IDC_SELECTED_OBJECT, 0, iY += 5, 170, 22, VK_F8, false, &g_ObjectModelSelectCombo);
    g_ObjectModelSelectCombo->AddItem(L"Teapot      ", ULongToPtr(E_TEAPOT));
    g_ObjectModelSelectCombo->AddItem(L"Mobius Strip", ULongToPtr(E_MOBIUS_STRIP));

    WCHAR sz[100];
    iY += 10;
    swprintf_s( sz, L"Patch Divisions: %2.1f", g_fSubdivs );
    g_SampleUI.AddStatic( IDC_PATCH_SUBDIVS_STATIC, sz, 10, iY += 26, 170, 22 );
    g_SampleUI.AddSlider( IDC_PATCH_SUBDIVS, 10, iY += 24, 170, 22, 10 * MIN_DIVS, 10 * MAX_DIVS, (int)(g_fSubdivs * 10) );
    swprintf_s( sz, L"Material Ka: %2.1f", g_fKa );
    g_SampleUI.AddStatic( IDC_PATCH_SUBDIVS_STATIC, sz, 10, iY += 26, 170, 22 );
    g_SampleUI.AddSlider( IDC_PATCH_SUBDIVS, 10, iY += 24, 170, 22, 10 * MIN_DIVS, 10 * MAX_DIVS, (int)(g_fKa * 10) );
    
    swprintf_s( sz, L"Material Kd: %2.1f", g_fKd );
    g_SampleUI.AddStatic( IDC_PATCH_SUBDIVS_STATIC, sz, 10, iY += 26, 170, 22 );
    g_SampleUI.AddSlider( IDC_PATCH_SUBDIVS, 10, iY += 24, 170, 22, 10 * MIN_DIVS, 10 * MAX_DIVS, (int)(g_fKd * 10) );

    swprintf_s( sz, L"Material Ks: %2.1f", g_fKs );
    g_SampleUI.AddStatic( IDC_PATCH_SUBDIVS_STATIC, sz, 10, iY += 26, 170, 22 );
    g_SampleUI.AddSlider( IDC_PATCH_SUBDIVS, 10, iY += 24, 170, 22, 10 * MIN_DIVS, 10 * MAX_DIVS, (int)(g_fKs * 10) );

    swprintf_s( sz, L"Material Shininess: %2.1f", g_fShininess );
    g_SampleUI.AddStatic( IDC_PATCH_SUBDIVS_STATIC, sz, 10, iY += 26, 170, 22 );
    g_SampleUI.AddSlider( IDC_PATCH_SUBDIVS, 10, iY += 24, 170, 22, 10 * MIN_DIVS, 10 * MAX_DIVS, (int)(g_fShininess * 10) );
    iY += 24;
    g_SampleUI.AddCheckBox( IDC_TOGGLE_LINES, L"Toggle Wires", 20, iY += 26, 150, 22, g_bDrawWires );
    g_SampleUI.AddCheckBox( IDC_SINGLE_PATCH, L"Single Patch", 20, iY += 26, 150, 22, g_bSinglePatch );
    g_SampleUI.AddCheckBox( IDC_TRI_DOMAIN, L"Tri Domain", 20, iY += 26, 150, 22, g_bTriDomain );

    iY += 24;
    g_SampleUI.AddRadioButton( IDC_PARTITION_INTEGER, IDC_PARTITION_MODE, L"Integer", 20, iY += 26, 170, 22 );
    g_SampleUI.AddRadioButton( IDC_PARTITION_FRAC_EVEN, IDC_PARTITION_MODE, L"Fractional Even", 20, iY += 26, 170, 22 );
    g_SampleUI.AddRadioButton( IDC_PARTITION_FRAC_ODD, IDC_PARTITION_MODE, L"Fractional Odd", 20, iY += 26, 170, 22 );
    g_SampleUI.GetRadioButton( IDC_PARTITION_INTEGER )->SetChecked( true );
#endif

    // Setup the camera's view parameters
    static const XMVECTORF32 s_vecEye = { 0.0f, .5f, -5.0f, 0.f };
    static const XMVECTORF32 s_vecAt  = { 0.0f, 0.0f,   0.0f, 0.f };
    CameraManager::getCamera().SetViewParams( s_vecEye, s_vecAt );

}

bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    return true;
}

void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    // Update the camera's position based on user input 
    CameraManager::getCamera().FrameMove( fElapsedTime );
}

static const WCHAR* UserOption()
{
    const RenderOption & option = RenderOption::getRenderOption();
    wchar_t wString[4096];
    wsprintf(wString, L"- wireframe: %d, diagModeOn:%d, tessellator factor:%d, height map:%d, diag type:%d ",
        option.wireframeOn, option.diagModeOn, option.tessellateFactor, option.heightMapOn, option.diagType);
    return wString;
}

void RenderText()
{
    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 2, 0 );
    g_pTxtHelper->SetForegroundColor( Colors::Yellow );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );
    g_pTxtHelper->DrawTextLine( UserOption() );
    g_pTxtHelper->End();
}

LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext )
{
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing ) return 0;

    // Pass messages to settings dialog if its active
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing ) return 0;
    *pbNoFurtherProcessing = g_SampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing ) return 0;

    // Pass all remaining windows messages to camera so it can respond to user input
    CameraManager::getCamera().HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
}

//--------------------------------------------------------------------------------------
void CALLBACK KeyboardProc(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext)
{
	if (bKeyDown)
	{
        char ch = tolower((int)nChar);
		switch (nChar)
		{
		    case VK_F1:
			    break;
		}
        switch (ch)
        {
            case 'w':
            {
                bool wire = RenderOption::getRenderOption().wireframeOn;
                RenderOption::getRenderOption().wireframeOn = !wire;
                break;
            }
            case 's':
            {
                bool diag = RenderOption::getRenderOption().diagModeOn;
                RenderOption::getRenderOption().diagModeOn = !diag;
                break;
            }
            case 't':
            {
                int wire = RenderOption::getRenderOption().tessellateFactor;
                RenderOption::getRenderOption().tessellateFactor = wire >= 64 ? 1 : wire * 2 > 64 ? wire + 1 : wire * 2;
                break;
            }
            case 'h':
            {
                unsigned int height = RenderOption::getRenderOption().heightMapOn;
                RenderOption::getRenderOption().heightMapOn = !height;
                break;
            }
            case 'd':
            {
                DiagType type = RenderOption::getRenderOption().diagType;
                RenderOption::getRenderOption().diagType = DiagType((type + 1) % DiagType::eDiagNum);
                break;
            }
        }
	}
}

//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    switch( nControlID )
    {
            // Standard DXUT controls
        case IDC_TOGGLEFULLSCREEN:
            DXUTToggleFullScreen(); break;
        case IDC_TOGGLEREF:
            DXUTToggleREF(); break;
        case IDC_CHANGEDEVICE:
            g_D3DSettingsDlg.SetActive( !g_D3DSettingsDlg.IsActive() ); break;

#if 0
            // Custom app controls
        case IDC_PATCH_SUBDIVS:
        {
            g_fSubdivs = g_SampleUI.GetSlider( IDC_PATCH_SUBDIVS )->GetValue() / 10.0f;

            WCHAR sz[100];
            swprintf_s( sz, L"Patch Divisions: %2.1f", g_fSubdivs );
            g_SampleUI.GetStatic( IDC_PATCH_SUBDIVS_STATIC )->SetText( sz );
        }
            break;
        case IDC_TOGGLE_LINES:
            g_bDrawWires = g_SampleUI.GetCheckBox( IDC_TOGGLE_LINES )->GetChecked();
            break;
        case IDC_SINGLE_PATCH:
            g_bSinglePatch = g_SampleUI.GetCheckBox( IDC_SINGLE_PATCH )->GetChecked();
            break;
        case IDC_TRI_DOMAIN:
            g_bTriDomain = g_SampleUI.GetCheckBox( IDC_TRI_DOMAIN )->GetChecked();
            break;
#endif
    }
}


//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                      void* pUserContext )
{
    HRESULT hr;

    auto pd3dImmediateContext = DXUTGetD3D11DeviceContext();
    V_RETURN( g_DialogResourceManager.OnD3D11CreateDevice( pd3dDevice, pd3dImmediateContext ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11CreateDevice( pd3dDevice ) );
    g_pTxtHelper = new CDXUTTextHelper( pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15 );

#if 0
    //BezierSurfaceManager::getBezierSurface().SetupMeshData(new UtahTeapot);
    BezierSurfaceManager::getBezierSurface().SetupMeshData(new Quad);
    BezierSurfaceManager::getBezierSurface().Initialize(pd3dDevice, pd3dImmediateContext);;
#else
    ShaderContainer &container = ShaderContainer::getShaderContainer();
    container.addShader(".\\shader\\drawCameraVector.hlsl");
    container.Init(pd3dDevice);
    
    CameraLayer::getCameraLayer().SetupMeshData(new EyePoint);
    CameraLayer::getCameraLayer().Initialize(pd3dDevice, pd3dImmediateContext);

#endif

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
    V_RETURN( g_D3DSettingsDlg.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

    // Setup the camera's projection parameters
    float fAspectRatio = pBackBufferSurfaceDesc->Width / ( FLOAT )pBackBufferSurfaceDesc->Height;
    CameraManager::getCamera().SetProjParams( XM_PI / 4, fAspectRatio, 0.1f, 600.0f );
    CameraManager::getCamera().SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );
    CameraManager::getCamera().SetButtonMasks( MOUSE_MIDDLE_BUTTON, MOUSE_WHEEL, MOUSE_LEFT_BUTTON );

    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
    g_HUD.SetSize( 170, 170 );
    g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height - 400 );
    g_SampleUI.SetSize( 170, 200 );

    return S_OK;
}




//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime, float fElapsedTime, void* pUserContext )
{
    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.OnRender( fElapsedTime );
        return;
    }

    ////////////////////////////////////////////////////////////////
    // Render teessellated Bezier Surface 
    ////////////////////////////////////////////////////////////////
    // Clear the render target and depth stencil
    ////////////////////////////////////////////////////////////////
    auto pRTV = DXUTGetD3D11RenderTargetView();
    pd3dImmediateContext->ClearRenderTargetView( pRTV, Colors::Black );
    auto pDSV = DXUTGetD3D11DepthStencilView();
    pd3dImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );

    //BezierSurfaceManager::getBezierSurface().Render(pd3dImmediateContext);

    //CameraLayer::getCameraLayer().Render(pd3dImmediateContext);

    ////////////////////////////////////////////////////////////////
    // Render the HUD
    ////////////////////////////////////////////////////////////////
    DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );
    g_HUD.OnRender( fElapsedTime );
    g_SampleUI.OnRender( fElapsedTime );
    RenderText();
    DXUT_EndPerfEvent();
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
    g_D3DSettingsDlg.OnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();                        
    //BezierSurfaceManager::getBezierSurface().DestroyD3D11Objects();

    CameraLayer::getCameraLayer().DestroyD3D11Objects();          
    ShaderContainer::getShaderContainer().Destory();
}
