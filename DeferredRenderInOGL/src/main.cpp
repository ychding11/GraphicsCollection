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


//Eanble High Performance GPU on Optimus devices
extern "C" {
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

using namespace std;

int g_width = 1280;
int g_height = 720;
static int g_winId;

objLoader g_meshloader;

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
    glutInitContextVersion( 4, 3 );
    glutInitContextFlags( GLUT_FORWARD_COMPATIBLE );
    glutInitContextProfile( GLUT_COMPATIBILITY_PROFILE );
    glutInitWindowSize( g_width, g_height );
    g_winId = glutCreateWindow( "Deferred Rendering in OGL" );

    cout << "OpenGL version " << glGetString(GL_VERSION) << " supported" << endl;

    GLenum err = glewInit();
    if( err != GLEW_OK )
    {
        cout << "ERROR: glewInit failed.\n";
        exit(1);
    }
    cout << "Status: Using GLEW " << glewGetString(GLEW_VERSION) << endl;

    HookGLutHandler();

    initShader();
    initFBO(g_width, g_height);

    LoadMesh(objects);
    initVertexData(); 

    initLight();

    glutMainLoop();
    FreeImage_DeInitialise();
    exit(0);
}