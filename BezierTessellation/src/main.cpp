#include <windows.h>
#include <d3d11.h>                                 
#include "Logger.h"
#include "BezierSurface.h"
#include "ShaderContainer.h"
#include "App.h"

static HWND hWnd = NULL;
static App* app = NULL;

#define CHECK_WIN_CALL_FAIL  0xffff

#define WIN_CALL_CHECK(x)                             \
do{                                                   \
    LRESULT ret = x;                                  \
    if((ret) != S_OK)                                 \
    {                                                 \
        char buf[512];                                \
        sprintf_s(buf, 512, "- Error @%s:%d\t  %s %d\t \n",__FILE__,__LINE__, #x, (ret) );  \
        OutputDebugStringA(buf);                      \
        system("pause");                              \
        return CHECK_WIN_CALL_FAIL;                   \
    }                                                 \
} while(0)


LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch( message )
	{
    	case WM_PAINT:
        {
	        PAINTSTRUCT ps;
	        HDC hdc;
    		hdc = BeginPaint( hWnd, &ps );
    		EndPaint( hWnd, &ps );
    		break;
        }
    	case WM_KEYUP:
        {
            char ch = tolower((int)wParam);
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
                case 'q':
                {
    		        PostQuitMessage( 0 );
                    break;
                }
            }
    		break;
        }
    	case WM_DESTROY:
        {
    		PostQuitMessage( 0 );
    		break;
        }
        case WM_SIZE:
        {
            if (wParam != SIZE_MINIMIZED)
                if (app) app->Resize(hWnd);
            break;
        }
    	default:
    		return DefWindowProc( hWnd, message, wParam, lParam );
	}
	return 0;
}

#define CLASS_NAME  L"TutorialWindowClass"
#define WINDOW_NAME L"HW Tessellation"

HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof( WNDCLASSEX );
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
	wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = CLASS_NAME;
	wcex.hIconSm = NULL;
	if( !RegisterClassEx( &wcex ) )
		return E_FAIL;

    int width  = (LONG)::GetSystemMetrics(SM_CXSCREEN);
    int height = (LONG)::GetSystemMetrics(SM_CYSCREEN);

	RECT rc = { 0, 0, width, height };
	AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
	hWnd = CreateWindow( CLASS_NAME, WINDOW_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance, NULL );
	if( !hWnd ) return E_FAIL;

	ShowWindow( hWnd, nCmdShow );
	return S_OK;
}

EyePoint   eye;
Quad       quad;
UtahTeapot teap;

TessSurface tessellator;

int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    WIN_CALL_CHECK( InitWindow( hInstance, nCmdShow ) );

    App* tess = &tessellator;  app = tess;
    tess->SetMesh(&quad);
    WIN_CALL_CHECK(tess->Initialize(hWnd));

    Camera& camera = CameraManager::getCamera();                               
    //camera.SetPosition(0.0f, 2.0f, -15.0f);           
    camera.LookAt({ 0.f, 1.f, -5.f, 0 }, { 0, 0, 0, 0 }, {0.f, 1.f, 0.f, 0.f});
    camera.SetLens(0.25f * sPi , 1.68f, 0.1f, 1000.0f);

    ///////////////////////////////////////////////////////
    /// Message Loop
    ///////////////////////////////////////////////////////
	MSG msg = { 0 };
	while( WM_QUIT != msg.message )
	{
		if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
		{
			tess->Render();
		}
	}
    ///////////////////////////////////////////////////////
    // Message Loop ---- End
    ///////////////////////////////////////////////////////

    tess->Destory();
    //Logger::flushLogger();
	return ( int )msg.wParam;
}