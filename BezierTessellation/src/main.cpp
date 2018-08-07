#include <windows.h>
#include <d3d11.h>                                 
#include "Logger.h"
#include "BezierSurface.h"
#include "ShaderContainer.h"
#include "App.h"

static HWND hWnd = NULL;

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
            }
    		break;
        }
    	case WM_DESTROY:
        {
    		PostQuitMessage( 0 );
    		break;
        }
    	default:
    		return DefWindowProc( hWnd, message, wParam, lParam );
	}
	return 0;
}

#define CLASS_NAME  L"TutorialWindowClass"
#define WINDOW_NAME L"Compute Shader - Filters"

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

int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
	if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
	{
        Logger::getLogger() << "Initialize window failed, exit." << "\n";
		return 0;
	}
    App & application = TessBezierSurface();
	if (!application.Initialize(hWnd))
	{
        Logger::getLogger() << "Initialize App failed, exit!" << "\n";
		return 0;
	}

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
			application.Render();
		}
	}

    application.Destory();
    Logger::flushLogger();
	return ( int )msg.wParam;
}