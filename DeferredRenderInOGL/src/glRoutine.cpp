// Sparse Voxel Octree and Voxel Cone Tracing
// 
// University of Pennsylvania CIS565 final project
// Camera control code originated from basecode offCIS565 project 6
// copyright (c) 2013 Cheng-Tso Lin  

#define GLM_SWIZZLE
#include <iostream>
#include <vector>
#include <gl/glew.h>
#include <gl/freeglut.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include "shader.h"
#include "glRoutine.h"
#include "objLoader.h"
#include "camera.h"
#include "variables.h"
#include "util.h"

using namespace std;
using namespace glm;

const int NUM_RENDERTARGET = 3;

mat4 modelview;
mat4 model;
mat4 view;
mat4 projection;
mat4 normalMat;

vec3 eyePos = vec3(0,0,0 );
vec3 eyeLook = vec3(-1,0,-1);
vec3 upDir = vec3(0,1,0);
Camera cam( eyePos, eyeLook, upDir );

float FOV = 60.0f;
float zNear = 0.05f;
float zFar = 10.0f;
float aspect;

//lighting
Light light1;

//Use this matrix to shift the shadow map coordinate
glm::mat4 biasMatrix(
0.5, 0.0, 0.0, 0.0,
0.0, 0.5, 0.0, 0.0,
0.0, 0.0, 0.5, 0.0,
0.5, 0.5, 0.5, 1.0
);


//Shader programs-rendering
shader::ShaderProgram renderVoxelShader;  //Scene-displaying shader program

//Shader programs-SVO construction
shader::ShaderProgram voxelizeShader; //Scene-voxelization shader program
shader::ComputeShader nodeFlagShader; 
shader::ComputeShader nodeAllocShader;
shader::ComputeShader nodeInitShader;
shader::ComputeShader leafStoreShader;
shader::ComputeShader mipmapNodeShader;

shader::ComputeShader octreeTo3DtexShader;


//Shader programs - deferred shading
shader::ShaderProgram passShader;
shader::ShaderProgram deferredShader;

//Shader programs - Shadow map 
shader::ShaderProgram shadowmapShader;

//OpenGL buffer objects for loaded mesh
GLuint vbo[10] = {0}; 
GLuint nbo[10] = {0};
GLuint tbo[10] = {0};
GLuint ibo[10] = {0};
GLuint vao[10] = {0};

const int QUAD = 8;
const int VOXEL3DTEX = 9;


//voxel dimension
int voxelDim = 128;
int octreeLevel = 7;
unsigned int numVoxelFrag = 0;

//voxel-creation rlated buffers
GLuint voxelTex = 0;   //3D texture
GLuint voxelPosTex = 0;  //texture for voxel fragment list (position)
GLuint voxelPosTbo = 0;  //texture buffer object for voxel fragment list (position)
GLuint voxelKdTex = 0;  //texture for voxel fragment list (diffuse)
GLuint voxelKdTbo = 0;  //texture buffer object for voxel fragment list (diffuse)
GLuint voxelNrmlTex = 0;  //texture for voxel fragment list (normal)
GLuint voxelNrmlTbo = 0;  //texture buffer object for voxel fragment list (normal)

GLuint atomicBuffer = 0;

//Octree pool buffer
GLuint octreeNodeTex[4] = {0}; //0: child node index
GLuint octreeNodeTbo[4] = {0};

//Textures for deferred shading
GLuint depthFBTex = 0;
GLuint normalFBTex = 0;
GLuint positionFBTex = 0;
GLuint colorFBTex = 0;
GLuint postFBTex = 0;

//Textures for shadowmaping
GLuint shadowmapTex = 0;

//Framebuffer objects
GLuint FBO[2] = {0,0}; //

static enum RenderMode render_mode = RENDER_FULL_SCENE; 
enum Display display_type = DISPLAY_NORMAL; 

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
    if( h == 0 || w == 0 )
        return;

    g_width  = w;
    g_height = h;
    glViewport( 0, 0, w, h );
    glBindFramebuffer(GL_FRAMEBUFFER,0);

    if (FBO[0] != 0 || depthFBTex != 0 || normalFBTex != 0 )
	{
        freeFBO();
    }
  
    initFBO(w,h);

    projection = glm::perspective( FOV, (float)w / (float)h, zNear, zFar );
    //projection = glm::ortho( -1.0f, 1.0f, -1.0f, 1.0f, zNear, zFar );
}

int mouse_buttons = 0;
int mouse_old_x = 0;
int mouse_old_y = 0;

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

static void renderGeometry()
{
    bindFBO(0);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );

    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );

    glEnable(GL_TEXTURE_2D);

    model = mat4(1.0);
    view  = cam.get_view();
    modelview = cam.get_view();
    normalMat = transpose( inverse( modelview ) );
    mat4 persp = perspective(45.0f,(float)g_width / (float)g_height, zNear, zFar);

    passShader.use();
    passShader.setParameter( shader::f1, (void*)&zFar,  "u_Far" );
    passShader.setParameter( shader::f1, (void*)&zNear, "u_Near" );
    passShader.setParameter( shader::mat4x4, (void*)&model[0][0], "u_Model" );
    passShader.setParameter( shader::mat4x4, (void*)&view[0][0], "u_View" );
    passShader.setParameter( shader::mat4x4, (void*)&persp[0][0], "u_Persp" );
    passShader.setParameter( shader::mat4x4, (void*)&normalMat[0][0], "u_InvTrans" );

    int bTextured;
    int numModel = g_meshloader.getModelCount();
    for( int i = 0; i < numModel; ++i )
    {
        const ObjModel* model = g_meshloader.getModel(i);
        glBindVertexArray( vao[i] );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo[i] );
        for( int i = 0; i < model->numGroup; ++i )
        {
            model->groups[i].shininess = 50;
            passShader.setParameter( shader::fv3, &model->groups[i].kd, "u_Color" );
            passShader.setParameter( shader::f1, &model->groups[i].shininess, "u_shininess" );
            if( model->groups[i].texId > 0 )
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, model->groups[i].texId );
                passShader.setTexParameter( 0, "u_colorTex" );
                bTextured = 1;
            }
            else
                bTextured = 0;
            passShader.setParameter( shader::i1, &bTextured, "u_bTextured" );

            if( model->groups[i].bumpTexId > 0 )
            {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, model->groups[i].bumpTexId );
                passShader.setTexParameter( 1, "u_bumpTex" );
                bTextured = 1;
            }
            else
                bTextured = 0;
            passShader.setParameter( shader::i1, &bTextured, "u_bBump" );

            glDrawElements( GL_TRIANGLES, 3 * model->groups[i].numTri , GL_UNSIGNED_INT, (void*)model->groups[i].ibo_offset );
        }
    }
}

static int geometryChanged = 1;
void renderScene()
{
    renderGeometry();

    //PASS 2: shadow map generation
    renderShadowMap( light1 );

    //PASS 3: shading
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glBindFramebuffer( GL_FRAMEBUFFER, 0);
    //glViewport( 10, 10, 400, 300);

    modelview = cam.get_view();
    vec4 lightPos = light1.pos;
    mat4 persp = perspective(45.0f,(float)g_width / (float)g_height, zNear, zFar);
    mat4 biasLightMVP = biasMatrix * light1.mvp;

    deferredShader.use();
    deferredShader.setParameter( shader::mat4x4, &biasLightMVP[0][0], "u_lightMVP" );
    deferredShader.setParameter( shader::mat4x4, &projection[0][0], "u_persp" );
    deferredShader.setParameter( shader::mat4x4, &modelview[0][0], "u_modelview" );

    deferredShader.setParameter( shader::i1, &g_height, "u_ScreenHeight" );
    deferredShader.setParameter( shader::i1, &g_width, "u_ScreenWidth" );
    deferredShader.setParameter( shader::f1, &zFar, "u_Far" );
    deferredShader.setParameter( shader::f1, &zNear, "u_Near" );
    deferredShader.setParameter( shader::mat4x4, &persp[0][0], "u_Persp" );
    deferredShader.setParameter( shader::i1, &display_type, "u_DisplayType" );
    deferredShader.setParameter( shader::i1, &render_mode, "u_RenderMode" );

    eyePos = cam.get_pos();
    deferredShader.setParameter( shader::fv4, &lightPos[0], "u_LightPos" );
    deferredShader.setParameter( shader::fv3, &light1.color[0], "u_LightColor" );
    deferredShader.setParameter( shader::fv3, &eyePos[0], "u_eyePos" );

    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthFBTex);
    deferredShader.setTexParameter( 0, "u_Depthtex" );

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalFBTex);
    deferredShader.setTexParameter( 1, "u_Normaltex" );

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, positionFBTex);
    deferredShader.setTexParameter( 2, "u_Positiontex" );

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, colorFBTex);
    deferredShader.setTexParameter( 3, "u_Colortex" );

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, shadowmapTex);
    deferredShader.setTexParameter( 4, "u_shadowmap" );

    //Draw the screen space quad
    glBindVertexArray( vao[QUAD] );
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo[QUAD] );
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT,0);

    glBindVertexArray(0);
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

    glEnable(GL_DEPTH_TEST);
}

void renderShadowMap( Light &light )
{
    glBindFramebuffer( GL_FRAMEBUFFER, FBO[1] );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glDisable( GL_CULL_FACE );
    glPolygonOffset(1.1f, 4.0f);
	
    shadowmapShader.use();

    mat4 lightProj = glm::perspective<float>( 60, 1, zNear, zFar );
    mat4 lightView = glm::lookAt( vec3(light.pos), vec3(0,0,0), vec3( 1,0,0) );
    mat4 lightModel = mat4(1.0);
    light.mvp = lightProj * lightView * lightModel;

    shadowmapShader.setParameter( shader::mat4x4, &light.mvp[0][0], "u_mvp" );

    int bTextured;
    int numModel = g_meshloader.getModelCount();
    for( int i = 0; i < numModel; ++i )
    {
        const ObjModel* model = g_meshloader.getModel(i);
        glBindVertexArray( vao[i] );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo[i] );
        for( int i = 0; i < model->numGroup; ++i )
        {
            model->groups[i].shininess = 50;
            shadowmapShader.setParameter( shader::fv3, &model->groups[i].kd, "u_Color" );
            shadowmapShader.setParameter( shader::f1, &model->groups[i].shininess, "u_shininess" );
            if( model->groups[i].texId > 0 )
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, model->groups[i].texId );
                shadowmapShader.setTexParameter( 0, "u_colorTex" );
                bTextured = 1;
            }
            else
                bTextured = 0;
            shadowmapShader.setParameter( shader::i1, &bTextured, "u_bTextured" );

            if( model->groups[i].bumpTexId > 0 )
            {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, model->groups[i].bumpTexId );
                shadowmapShader.setTexParameter( 1, "u_bumpTex" );
                bTextured = 1;
            }
            else
                bTextured = 0;
            shadowmapShader.setParameter( shader::i1, &bTextured, "u_bBump" );

            glDrawElements( GL_TRIANGLES, 3 * model->groups[i].numTri , GL_UNSIGNED_INT, (void*)model->groups[i].ibo_offset );
        }
    }
    glPolygonOffset(0,0);
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
}

void initFBO( int w, int h )
{
    GLenum FBOstatus;
    GLenum err;

    glActiveTexture(GL_TEXTURE9);

    depthFBTex  = gen2DTexture( w, h, GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_FLOAT );
    normalFBTex = gen2DTexture( w, h, GL_RGBA32F, GL_RGBA, GL_FLOAT );
    positionFBTex = gen2DTexture( w, h, GL_RGBA32F, GL_RGBA, GL_FLOAT );
    colorFBTex    = gen2DTexture( w, h, GL_RGBA32F, GL_RGBA, GL_FLOAT );

    // create a framebuffer object
    glGenFramebuffers(1, &FBO[0]);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO[0]);

    // Instruct openGL that we won't bind a color texture with the currently bound FBO
    glReadBuffer(GL_NONE);

    GLenum draws [NUM_RENDERTARGET];
    draws[0] = GL_COLOR_ATTACHMENT0;
    draws[1] = GL_COLOR_ATTACHMENT1;
    draws[2] = GL_COLOR_ATTACHMENT2;

    glDrawBuffers(NUM_RENDERTARGET, draws);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthFBTex, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, draws[0], normalFBTex, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, draws[1], positionFBTex, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, draws[2], colorFBTex, 0);
    
    // check FBO status
    FBOstatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(FBOstatus != GL_FRAMEBUFFER_COMPLETE)
	{
        printf(" - GL_FRAMEBUFFER_COMPLETE failed, CANNOT use FBO[0]\n");
        checkFramebufferStatus(FBOstatus);
    }
   
    //Shadow map buffers
    glGenTextures(1, &shadowmapTex );
    glBindTexture(GL_TEXTURE_2D, shadowmapTex );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32 , w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0 );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    
    glGenFramebuffers(1, &FBO[1]);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO[1]);
    glFramebufferTexture( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowmapTex, 0 );
    glDrawBuffer( GL_NONE ); //Disable render

    FBOstatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(FBOstatus != GL_FRAMEBUFFER_COMPLETE)
	{
        printf("GL_FRAMEBUFFER_COMPLETE failed, CANNOT use FBO[1]\n");
        checkFramebufferStatus(FBOstatus);
    }
    err = glGetError();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void freeFBO() 
{
    glDeleteTextures(1,&depthFBTex);
    glDeleteTextures(1,&normalFBTex);
    glDeleteTextures(1,&positionFBTex);
    glDeleteTextures(1,&colorFBTex);
    glDeleteFramebuffers(1,&FBO[0]);
    glDeleteFramebuffers(1,&FBO[1]);
}

void bindFBO(int buf)
{
    glBindFramebuffer(GL_FRAMEBUFFER, FBO[buf]);
}

void initShader()
{
    passShader.init( "shader/pass.vert.glsl", "shader/pass.frag.glsl" );
    deferredShader.init( "shader/shade.vert.glsl", "shader/diagnostic.frag.glsl" );
    shadowmapShader.init( "shader/shadowmap.vert.glsl", "shader/shadowmap.frag.glsl" );
}

void initVertexData()
{
    int numModel = g_meshloader.getModelCount();
    for( int i = 0; i < numModel; ++i )
    {
        const ObjModel* model = g_meshloader.getModel(i);

        glGenVertexArrays( 1, &vao[i] );
        glBindVertexArray( vao[i] );

        glGenBuffers( 1, &vbo[i] );
        glBindBuffer( GL_ARRAY_BUFFER, vbo[i] );
        glBufferData( GL_ARRAY_BUFFER, sizeof(float) * 3 * model->numVert, model->vbo, GL_STATIC_DRAW  );

        glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, 0 );
        glEnableVertexAttribArray( 0 );

        if( model->numNrml > 0 )
        {
            glGenBuffers( 1, &nbo[i] );
            glBindBuffer( GL_ARRAY_BUFFER, nbo[i] );
            glBufferData( GL_ARRAY_BUFFER, sizeof(float) * 3 * model->numVert, model->nbo, GL_STATIC_DRAW  );

            glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 0, 0 );
            glEnableVertexAttribArray( 1 );
        }
        if( model->numTxcoord > 0 )
        {
            glGenBuffers( 1, &tbo[i] );
            glBindBuffer( GL_ARRAY_BUFFER, tbo[i] );
            glBufferData( GL_ARRAY_BUFFER, sizeof(float) * 2 * model->numVert, model->tbo, GL_STATIC_DRAW  );

            glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, 0, 0 );
            glEnableVertexAttribArray( 2 );
        }

        glGenBuffers( 1, &ibo[i] );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo[i] );
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( unsigned int ) * model->numIdx, model->ibo, GL_STATIC_DRAW );

        for( int i = 0; i < model->numGroup; ++i )
        {
            if( model->groups[i].tex_filename.length() > 0 )
                model->groups[i].texId = loadTexturFromFile( model->groups[i].tex_filename.c_str(), GL_RGB8, GL_BGR, 2 );
            else
                model->groups[i].texId = 0;
            if( model->groups[i].bump_filename.length() > 0 )
                model->groups[i].bumpTexId = loadTexturFromFile( model->groups[i].bump_filename.c_str(), GL_RGB8, GL_BGR, 0 );
            else
                model->groups[i].bumpTexId = 0;
        }
    }
    
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    glBindVertexArray(0);

    createScreenQuad();
}

unsigned int gen2DTexture( int w, int h, GLenum internalFormat,  GLenum format, GLenum type )
{
    GLuint texId;
    glGenTextures(1, &texId);
    glBindTexture( GL_TEXTURE_2D, texId );
    glTexImage2D( GL_TEXTURE_2D, 0, internalFormat, w, h, 0, format, type, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
    glBindTexture( GL_TEXTURE_2D, 0 );
    return texId;
}

int genLinearBuffer( int size, GLenum format, GLuint* tex, GLuint* tbo )
{
    GLenum err;
   
    if( (*tbo) > 0 )
        glDeleteBuffers( 1, tbo );  //delete previously created tbo

    glGenBuffers( 1, tbo );
    glBindBuffer( GL_TEXTURE_BUFFER, *tbo );
    glBufferData( GL_TEXTURE_BUFFER, size, 0, GL_STATIC_DRAW );
    err = glGetError();

    if( (*tex) > 0 )
        glDeleteTextures( 1, tex ); //delete previously created texture

    glGenTextures( 1, tex );
    glBindTexture( GL_TEXTURE_BUFFER, *tex );
    glTexBuffer( GL_TEXTURE_BUFFER, format,  *tbo );

    glBindBuffer( GL_TEXTURE_BUFFER, 0 );

    err = glGetError();
    if( err > 0 )
        cout<<glewGetErrorString(err)<<endl;
    return err;
}

void genAtomicBuffer( int num, unsigned int &buffer )
{
    GLuint initVal = 0;

    if( buffer )
        glDeleteBuffers( 1, &buffer );
    glGenBuffers( 1, &buffer );
    glBindBuffer( GL_ATOMIC_COUNTER_BUFFER, buffer );
    glBufferData( GL_ATOMIC_COUNTER_BUFFER, sizeof( GLuint ), &initVal, GL_STATIC_DRAW );
    glBindBuffer( GL_ATOMIC_COUNTER_BUFFER, 0 );

}

//create a screen-size quad
void createScreenQuad()
{
    GLenum err;
    vertex2_t verts [] =
    {
		{vec3(-1, 1,0),vec2(0,1)},
        {vec3(-1,-1,0),vec2(0,0)},
        {vec3( 1,-1,0),vec2(1,0)},
        {vec3( 1, 1,0),vec2(1,1)}};

    unsigned short indices[] = { 0,1,2,0,2,3};

    glGenVertexArrays( 1, &vao[QUAD] );
    glBindVertexArray( vao[QUAD] );

    glGenBuffers(1,&vbo[QUAD]);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[QUAD]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,sizeof(vertex2_t),0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,sizeof(vertex2_t),(void*)sizeof(vec3));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glGenBuffers(1,&ibo[QUAD]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo[QUAD]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6*sizeof(GLushort), indices, GL_STATIC_DRAW);

    glBindVertexArray(0);

    err = glGetError();
}

void initLight()
{
    light1.pos = light1.initialPos = vec4( 0, 0.53, 0, 1.0 );
    light1.color = vec3( 1, 1, 1 );
}