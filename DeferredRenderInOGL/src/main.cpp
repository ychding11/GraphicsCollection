// Sparse Voxel Octree and Voxel Cone Tracing
// 
// University of Pennsylvania CIS565 final project
// copyright (c) 2013 Cheng-Tso Lin  

#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <gl/glew.h>
#include <gl/freeglut.h>
#include <glm/glm.hpp>
#include <glm.h>
#include "glRoutine.h"
#include "objLoader.h"
#include "variables.h"
#include "FreeImage.h"

#include "camera.h"

//Eanble High Performance GPU on Optimus devices
extern "C" {
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

using namespace std;

int g_width = 1280;
int g_height = 720;

objLoader g_meshloader;

Camera camera;

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
    renderScene();
    glutSwapBuffers();
}

void glut_idle()
{
    glutPostRedisplay();
}

void glut_reshape(int w, int h)
{
    if (h == 0 || w == 0) return;

    g_width = w;
    g_height = h;

    freeFBO();
    initFBO(w, h);

    if (h > 0)
    {
        window.size = ivec2(w, h);
        window.window_aspect = float(w) / float(h);
    }
    camera.SetViewport(0, 0, window.size.x, window.size.y);
}

void glut_mouse(int button, int state, int x, int y)
{
    camera.SetPos(button, state, x, y);
}

void glut_motion(int x, int y)
{
    camera.Move2D(x, y);
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
        render_mode = RENDER_FULL_SCENE;
        break;
    case '2':
        glutSetWindowTitle("Position");
        render_mode = RENDER_POSITION;
        break;
    case '3':
        glutSetWindowTitle("Normal");
        render_mode = RENDER_NORMAL;
        break;
    case '4':
        glutSetWindowTitle("Color");
        render_mode = RENDER_COLOR;
        break;
    case '5':
        glutSetWindowTitle("Shadow");
        render_mode = RENDER_SHADOW;
        break;
    case ('n'):
        glutSetWindowTitle("Full Scene With Normal");
        render_mode = RENDER_FULL_SCENE;
        break;
    case 'w':
        camera.Move(FORWARD);
        break;
    case 'a':
        camera.Move(LEFT);
        break;
    case 's':
        camera.Move(BACK);
        break;
    case 'd':
        camera.Move(RIGHT);
        break;
    case 'q':
        camera.Move(DOWN);
        break;
    case 'e':
        camera.Move(UP);
        break;
    case 'u':
        cout << (" - Regenerate Shader program objects. \n");
        initShader();
        break;
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
        cout << "Usage: mesh=[obj file]" << endl;
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

    //Setup camera
    camera.SetMode(FREE);
    camera.SetPosition(glm::vec3(0, 0, -1));
    camera.SetLookAt(glm::vec3(0, 0, 0));
    camera.SetClipping(.1, 1000);
    camera.SetFOV(45);

    initLight();
    initShader();
    initFBO(g_width, g_height);

    LoadMesh(objects);
    initVertexData(); 


    glutMainLoop();
    FreeImage_DeInitialise();
    exit(0);
}