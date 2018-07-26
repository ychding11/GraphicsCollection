#ifndef _GLROUTINE_H
#define _GLROUTINE_H

typedef struct
{
	glm::vec3 pt;
	glm::vec2 texcoord;

} vertex2_t;

enum Display
{
    DISPLAY_DEPTH = 0,
    DISPLAY_NORMAL = 1,
    DISPLAY_POSITION = 2,
    DISPLAY_COLOR = 3,
    DISPLAY_TOTAL = 4,
    DISPLAY_LIGHTS = 5,
    DISPLAY_TOON = 6
};

enum RenderMode
{
    RENDER_FULL_SCENE = 1,
    RENDER_POSITION   = 2,
    RENDER_NORMAL     = 3,
    RENDER_COLOR      = 4,
    RENDER_SHADOW     = 5,
};

enum UIOption
{
    UIOPTION_SHADOW_OFF = 1,
    UIOPTION_SHOW_NORMAL = 2,
};

enum LightType
{ 
    DIRECTIONAL = 1,
    PROJECTIVE = 2,
    POINTED = 3,
};

struct Light
{
    glm::vec4 initialPos;
    glm::vec4 pos;
    glm::vec3 color;
    glm::mat4 mvp; //each light has its own projection matrix
};


//////////////// GLUT callback functions
void glut_display();
void glut_reshape( int w, int h );
void glut_idle();
void glut_mouse( int button, int state, int x, int y );
void glut_motion( int x, int y );
void glut_keyboard( unsigned char key, int x, int y );
////////////////

void initShader();
void initVertexData();
void initLight();

// FBO
void initFBO( int w, int h );
void freeFBO();


///rendering routines
void renderScene();
static void renderShadowMap( Light &light );

///2D texture generation
static unsigned int gen2DTexture( int w, int h, GLenum internalFormat,  GLenum format, GLenum type );
static          int genLinearBuffer( int size, GLenum format, GLuint* tex, GLuint* tbo );

#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include "camera.h"

/// global variable
extern Camera camera;
extern enum RenderMode render_mode;

extern glm::mat4 world;
extern glm::mat4 view;
extern glm::mat4 projection;
#endif