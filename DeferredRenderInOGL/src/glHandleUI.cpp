#define GLM_SWIZZLE

#include <iostream>
#include <vector>
#include <gl/glew.h>
#include <gl/freeglut.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include "glRoutine.h"
//#include "objLoader.h"
#include "camera.h"
#include "variables.h"
//#include "util.h"

using namespace std;
using namespace glm;

void glut_display()
{
    renderScene(); 
    glutSwapBuffers();
}

void glut_idle()
{
    glutPostRedisplay();
}

void glut_reshape( int w, int h )
{
    if( h == 0 || w == 0 ) return;

    g_width  = w;
    g_height = h;
    glViewport( 0, 0, w, h );
    glBindFramebuffer(GL_FRAMEBUFFER,0);

    freeFBO();
    initFBO(w,h);

    projection = glm::perspective( FOV, (float)w / (float)h, zNear, zFar );
    //projection = glm::ortho( -1.0f, 1.0f, -1.0f, 1.0f, zNear, zFar );
}

static int mouse_buttons = 0;
static int mouse_old_x = 0;
static int mouse_old_y = 0;

void glut_mouse( int button, int state, int x, int y )
{
    if (state == GLUT_DOWN) 
    {
        mouse_buttons |= 1<<button; 
    } 
    else if (state == GLUT_UP) 
    {
        mouse_buttons = 0;
    }
    mouse_old_x = x;
    mouse_old_y = y;
}

void glut_motion( int x, int y )
{
    float dx, dy;
    dx = (float)(x - mouse_old_x);
    dy = (float)(y - mouse_old_y);

    if (mouse_buttons & 1<<GLUT_RIGHT_BUTTON)
	{
        cam.adjust(0,0,dx,0,0,0);;
    }
    else
	{
        cam.adjust(-dx*0.2f,-dy*0.2f,0.0f,0,0,0);
    }
    mouse_old_x = x;
    mouse_old_y = y;
}

void glut_keyboard( unsigned char key, int x, int y )
{
    float tx = 0;
    float ty = 0;
    float tz = 0;
    switch(key)
	{
        case (27):
            exit(0);
            break;
        case '1':
            glutSetWindowTitle("Full Scene" );
            render_mode = RENDER_FULL_SCENE;
            break;
        case '2':
            glutSetWindowTitle("Position" );
            render_mode = RENDER_POSITION;
            break;
        case '3':
            glutSetWindowTitle("Normal" );
            render_mode = RENDER_NORMAL;
            break;
        case '4':
            glutSetWindowTitle("Color" );
            render_mode = RENDER_COLOR;
            break;
        case '5':
            glutSetWindowTitle("Shadow" );
            render_mode = RENDER_SHADOW;
            break;
        case ('n'):
            glutSetWindowTitle("Full Scene With Normal" );
            render_mode = RENDER_FULL_SCENE;
            break;
        case ('w'):
            tz = -0.01f;
            break;
        case ('s'):
            tz = 0.01f;
            break;
        case ('d'):
            tx = -0.01f;
            break;
        case ('a'):
            tx = 0.01f;
            break;
        case ('q'):
            ty = 0.01f;
            break;
        case ('z'):
            ty = -0.01f;
            break;
        case 'u':
			printf(" - Regenerate Shader program objects. \n");
            initShader();
            break;
    }
    if (abs(tx) > 0 ||  abs(tz) > 0 || abs(ty) > 0)
	{
        cam.adjust(0,0,0,tx,ty,tz);
       
    }
}
