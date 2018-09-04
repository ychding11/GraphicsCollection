// Example to test the architecute under desing 

#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <gl/glew.h>
#include <gl/freeglut.h>
#include <glm/glm.hpp>
#include <glm.h>
#include "objLoader.h"
#include "variables.h"
#include "FreeImage.h"

#include "camera.h"
#include "Renderer.h"
#include "ShaderManager.h"

//Eanble High Performance GPU on Optimus devices
extern "C" {
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

//#define UGLY

using namespace std;

int g_width = 1280;
int g_height = 720;

objLoader g_meshloader;

class Window {
public:
    Window()
    {
        this->interval = 1000 / 60;		//60 FPS
        this->window_handle = -1;
    }
    int window_handle, interval;
    ivec2 size;
    float window_aspect;
} window;

void glut_display()
{
    RendererManager::getRender().Render();
    glutSwapBuffers();
}

void glut_idle()
{
    glutPostRedisplay();
}

void glut_reshape(int w, int h)
{
    RendererManager::getRender().UpdateBackBuffer(w, h);
    if (h > 0)
    {
        window.size = ivec2(w, h);
        window.window_aspect = float(w) / float(h);
    }
    CameraManager::getCamera("draw").SetViewport(0, 0, window.size.x, window.size.y);
}

void glut_mouse(int button, int state, int x, int y)
{
    CameraManager::getCamera("draw").SetPos(button, state, x, y);
}

void glut_motion(int x, int y)
{
    CameraManager::getCamera("draw").Move2D(x, y);
}

void glut_keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
    case (27):
        exit(0);
        break;
    case '1':
        glutSetWindowTitle("Full Scene");
        break;
    case '2':
        glutSetWindowTitle("Position");
        break;
    case '3':
        glutSetWindowTitle("Normal");
        break;
    case '4':
        glutSetWindowTitle("Color");
        break;
    case '5':
        glutSetWindowTitle("Shadow");
        break;
    case 'w':
        CameraManager::getCamera("draw").Move(FORWARD);
        break;
    case 'a':
        CameraManager::getCamera("draw").Move(LEFT);
        break;
    case 's':
        CameraManager::getCamera("draw").Move(BACK);
        break;
    case 'd':
        CameraManager::getCamera("draw").Move(RIGHT);
        break;
    case 'q':
        CameraManager::getCamera("draw").Move(DOWN);
        break;
    case 'e':
        CameraManager::getCamera("draw").Move(UP);
        break;
    case 'v':
    {
        bool view = RenderOption::getRenderOption().deferred;
        RenderOption::getRenderOption().deferred= !view;
        cout << "- keyboard: deferred Rendering: " << RenderOption::getRenderOption().deferred<< std::endl;
        break;
    }
    case 'f':
    {
        bool wire = RenderOption::getRenderOption().wireframe;
        RenderOption::getRenderOption().wireframe= !wire;
        cout << "- keyboard: wireframe: " << RenderOption::getRenderOption().wireframe << std::endl;
        break;
    }
    case 'n': //draw normal
    {
        bool normal = RenderOption::getRenderOption().drawVnormal;
        RenderOption::getRenderOption().drawVnormal = !normal;
        cout << "- keyboard: wireframe: " << RenderOption::getRenderOption().drawVnormal << std::endl;
        break;
    }
    case 'c'://GLUT_KEY_F1: //Diag
    {
        DebugType diag = RenderOption::getRenderOption().diagType;
        RenderOption::getRenderOption().diagType = DebugType( (diag + 1) % DebugType::DEBUG_NUM );
        cout << "- keyboard: debug type: " << RenderOption::getRenderOption().diagType << std::endl;
        break;
    }
    case 'u':
    {
        cout << (" - Regenerate Shader program objects. \n");
        break;

    }
    }
}

static void LoadMesh(vector<string> &objects)
{
    for( size_t i = 0; i < objects.size(); ++i )
    {
        int ret = g_meshloader.load( objects[i] );
        if (ret == 0)
        {
            static int input;
            cout << "- Load Mesh failed:  mesh=" << objects[i] << "\n input any key to exit!" << endl;
            scanf("%d", &input);
            exit(1);
        }
    }
}

static void HookGLutHandler(void)
{
    glutDisplayFunc( glut_display );
    glutIdleFunc( glut_idle );
    glutReshapeFunc( glut_reshape );
    glutKeyboardFunc( glut_keyboard );
    glutMouseFunc( glut_mouse );
    glutMotionFunc( glut_motion );
}



int  main( int argc, char* argv[] )
{
    vector<string> objects;
    if( argc <= 1 )
    {
        cout << "Usage: mesh=[model file path]" << endl;
        return 0;
    }       
    for( int i = 1; i < argc; ++i )
    {
        string header;
        string data;
        istringstream line(argv[i]);
        getline( line, header, '=' );
        getline( line, data, '=' );
        if( header.compare( "mesh" ) == 0 )
        {
            objects.push_back(data);
        }
    }

    if( objects.size() == 0 )
    {
        cout << "- No Mesh specified. Usage: pargarm_name mesh=[obj file]" << endl;
        return 0;
    }

    FreeImage_Initialise();

    glutInit( &argc, argv );
    glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH );
    glutInitWindowPosition(0, 0);
    glutInitContextVersion( 4, 3 );
    glutInitContextFlags( GLUT_FORWARD_COMPATIBLE );
    glutInitContextProfile( GLUT_COMPATIBILITY_PROFILE );
    glutInitWindowSize( g_width, g_height );
    window.window_handle = glutCreateWindow( "Deferred Rendering in OGL" );

    cout << "OpenGL version " << glGetString(GL_VERSION) << " supported" << endl;

    GLenum err = glewInit();
    if( err != GLEW_OK )
    {
        cout << "ERROR: glewInit failed.\n";
        exit(1);
    }
    cout << "Status: Using GLEW " << glewGetString(GLEW_VERSION) << endl;

    HookGLutHandler();

    CameraManager::getCamera("draw").SetMode(FREE);
    CameraManager::getCamera("draw").SetPosition(glm::vec3(0, 0, 2));
    CameraManager::getCamera("draw").SetLookAt(glm::vec3(0, 0, 0));
    CameraManager::getCamera("draw").SetClipping(.1, 100);
    CameraManager::getCamera("draw").SetFOV(90);
    CameraManager::getCamera("draw").SetViewport(0, 0, window.size.x, window.size.y);

    shader::ShaderManager::getShaderManager().SetupShaders();
    LoadMesh(objects);
    RendererManager::getRender().SetUp(&g_meshloader);

    //glut loop
    glutMainLoop();
    FreeImage_DeInitialise();
    exit(0);
}