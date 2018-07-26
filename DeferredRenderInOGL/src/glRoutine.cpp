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


mat4 world = mat4(1.0);
mat4 view  = mat4(1.0);
mat4 projection = mat4(1.0);
mat4 lightProj;

mat4 normalToWorld = transpose( inverse( world ) );
mat4 normalToView  = transpose( inverse( view * world) );

//lighting
Light light1;

//Use this matrix to shift the shadow map coordinate
glm::mat4 biasMatrix(
0.5, 0.0, 0.0, 0.0,
0.0, 0.5, 0.0, 0.0,
0.0, 0.0, 0.5, 0.0,
0.5, 0.5, 0.5, 1.0
);

////////////////////////////////////////////////////////////////
////Shader Program Objects
////////////////////////////////////////////////////////////////
shader::ShaderProgram geometryShader;
shader::ShaderProgram phongShader;
shader::ShaderProgram shadowmapShader;
shader::ShaderProgram visualNormalShader;
shader::ShaderProgram plainShader;

////////////////////////////////////////////////////////////////
////OpenGL buffer objects for loaded mesh
////////////////////////////////////////////////////////////////
static GLuint vao[10] = {0};
static GLuint vbo[10] = {0}; 
static GLuint nbo[10] = {0};
static GLuint tbo[10] = {0};
static GLuint ibo[10] = {0};

static const int QUAD = 8;

////////////////////////////////////////////////////////////////
////Textures 
////////////////////////////////////////////////////////////////
static GLuint positionFBTex = 0;
static GLuint normalFBTex = 0;
static GLuint colorFBTex = 0;
static GLuint depthFBTex = 0;

static GLuint shadowmapTex = 0;

////////////////////////////////////////////////////////////////
////Framebuffer objects
////////////////////////////////////////////////////////////////
static GLuint FBO[2] = {0,0}; //

enum RenderMode render_mode = RENDER_FULL_SCENE; 
enum Display display_type = DISPLAY_NORMAL; 

////////////////////////////////////////////////////////////////
////  Resources allocated.
////
////
////////////////////////////////////////////////////////////////

static GLuint  gen2DTexture( int w, int h, GLenum internalFormat,  GLenum format, GLenum type )
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

static int genLinearBuffer( int size, GLenum format, GLuint* tex, GLuint* tbo )
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

static void genAtomicBuffer( int num, unsigned int &buffer )
{
    GLuint initVal = 0;

    if( buffer )
        glDeleteBuffers( 1, &buffer );
    glGenBuffers( 1, &buffer );
    glBindBuffer( GL_ATOMIC_COUNTER_BUFFER, buffer );
    glBufferData( GL_ATOMIC_COUNTER_BUFFER, sizeof( GLuint ), &initVal, GL_STATIC_DRAW );
    glBindBuffer( GL_ATOMIC_COUNTER_BUFFER, 0 );

}

////////////////////////////////////////////////////////////////
////  Render Pass related. 
////
////
////////////////////////////////////////////////////////////////

static void renderGeometryMap()
{
    glBindFramebuffer(GL_FRAMEBUFFER, FBO[0]);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );

    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );

    glEnable(GL_TEXTURE_2D);

    geometryShader.use();
    geometryShader.setParameter( shader::mat4x4, (void*)&world[0][0], "u_World");
    geometryShader.setParameter( shader::mat4x4, (void*)&view[0][0],  "u_View" );
    geometryShader.setParameter( shader::mat4x4, (void*)&projection[0][0], "u_Projection" );
    geometryShader.setParameter( shader::mat4x4, (void*)&normalToWorld[0][0], "u_NormalToWorld" );
    geometryShader.setParameter( shader::mat4x4, (void*)&normalToView[0][0], "u_NormalToView" );

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
            geometryShader.setParameter( shader::fv3, &model->groups[i].kd, "u_Color" );
            geometryShader.setParameter( shader::f1, &model->groups[i].shininess, "u_shininess" );
            if( model->groups[i].texId > 0 )
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, model->groups[i].texId );
                geometryShader.setTexParameter( 0, "u_colorTex" );
                bTextured = 1;
            }
            else
                bTextured = 0;
            geometryShader.setParameter( shader::i1, &bTextured, "u_bTextured" );

            if( model->groups[i].bumpTexId > 0 )
            {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, model->groups[i].bumpTexId );
                geometryShader.setTexParameter( 1, "u_bumpTex" );
                bTextured = 1;
            }
            else
                bTextured = 0;
            geometryShader.setParameter( shader::i1, &bTextured, "u_bBump" );

            glDrawElements( GL_TRIANGLES, 3 * model->groups[i].numTri , GL_UNSIGNED_INT, (void*)model->groups[i].ibo_offset );
        }
    }
}

static void renderNormal()
{
    //glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );
    //glEnable( GL_TEXTURE_2D );
    //glDepthFunc( GL_LESS );
    glCullFace( GL_BACK );

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    float normalStrength = .05f;

    // Update Shader Parameter
    visualNormalShader.use();
    visualNormalShader.setParameter( shader::f1, (void*)&normalStrength,  "u_NormalLength" );
    visualNormalShader.setParameter( shader::mat4x4, (void*)&world[0][0], "u_World");
    visualNormalShader.setParameter( shader::mat4x4, (void*)&view[0][0],  "u_View" );
    visualNormalShader.setParameter( shader::mat4x4, (void*)&projection[0][0], "u_Projection" );
    visualNormalShader.setParameter( shader::mat4x4, (void*)&normalToWorld[0][0], "u_NormalToWorld" );
    visualNormalShader.setParameter( shader::mat4x4, (void*)&normalToView[0][0],  "u_NormalToView" );

    int numModel = g_meshloader.getModelCount();
    for( int i = 0; i < numModel; ++i )
    {
        glBindVertexArray( vao[i] );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo[i] );
        const ObjModel* model = g_meshloader.getModel(i);
        for( int i = 0; i < model->numGroup; ++i )
        {
            glDrawElements( GL_TRIANGLES, 3 * model->groups[i].numTri , GL_UNSIGNED_INT, (void*)model->groups[i].ibo_offset );
        }
    }


    plainShader.use();
    plainShader.setParameter( shader::mat4x4, (void*)&world[0][0], "u_World");
    plainShader.setParameter( shader::mat4x4, (void*)&view[0][0],  "u_View" );
    plainShader.setParameter( shader::mat4x4, (void*)&projection[0][0], "u_Projection" );
    plainShader.setParameter( shader::mat4x4, (void*)&normalToWorld[0][0], "u_NormalToWorld" );
    plainShader.setParameter( shader::mat4x4, (void*)&normalToView[0][0],  "u_NormalToView" );

    for( int i = 0; i < numModel; ++i )
    {
        glBindVertexArray( vao[i] );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo[i] );
        const ObjModel* model = g_meshloader.getModel(i);
        for( int i = 0; i < model->numGroup; ++i )
        {
            glDrawElements( GL_TRIANGLES, 3 * model->groups[i].numTri , GL_UNSIGNED_INT, (void*)model->groups[i].ibo_offset );
        }
    }
}

static void LightShade()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glBindFramebuffer( GL_FRAMEBUFFER, 0);
    //glViewport( 10, 10, 400, 300);

    mat4 biasLightMVP = biasMatrix * light1.mvp;

    phongShader.use();
    phongShader.setParameter( shader::mat4x4, &view[0][0], "u_view" );
    phongShader.setParameter( shader::mat4x4, &projection[0][0], "u_projection" );

    phongShader.setParameter( shader::i1, &g_height, "u_ScreenHeight" );
    phongShader.setParameter( shader::i1, &g_width,  "u_ScreenWidth" );
    phongShader.setParameter( shader::i1, &render_mode, "u_RenderMode" );

    //eyePos = cam.get_pos();
    phongShader.setParameter( shader::fv4, &light1.pos[0],   "u_LightPos" );
    phongShader.setParameter( shader::fv3, &light1.color[0], "u_LightColor" );
    phongShader.setParameter( shader::mat4x4, &biasLightMVP[0][0], "u_lightMVP" );

    phongShader.setParameter( shader::fv3, &camera.camera_position[0], "u_eyePos" );

    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthFBTex);
    phongShader.setTexParameter( 0, "u_Depthtex" );

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalFBTex);
    phongShader.setTexParameter( 1, "u_Normaltex" );

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, positionFBTex);
    phongShader.setTexParameter( 2, "u_Positiontex" );

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, colorFBTex);
    phongShader.setTexParameter( 3, "u_Colortex" );

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, shadowmapTex);
    phongShader.setTexParameter( 4, "u_shadowmap" );

    //Draw the screen space quad
    glBindVertexArray( vao[QUAD] );
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo[QUAD] );
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT,0);

}

void renderShadowMap( Light &light )
{
    glBindFramebuffer( GL_FRAMEBUFFER, FBO[1] );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glDisable( GL_CULL_FACE );
    glEnable(GL_DEPTH_TEST);
    glPolygonOffset(1.1f, 4.0f);
	
    shadowmapShader.use();

    mat4 lightProj = glm::perspective<float>( 60, 1, .01, 1000. );
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

static void UpdateCameraParam()
{
    mat4 model;
    camera.Update();
    camera.GetMatricies(projection, view, model);
    normalToView = transpose( inverse( view * world) );
}

////////////////////////////////////////////////////////////////
//// Public interface
//// Main Render Pass
////////////////////////////////////////////////////////////////
void renderScene()
{
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
   // renderGeometryMap();
   // renderShadowMap( light1 );
   // LightShade();

    UpdateCameraParam();
    renderNormal();

    glBindVertexArray(0);
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}

static void initGeometryMapFBO(int w, int h)
{
    GLenum FBOstatus;

    glActiveTexture(GL_TEXTURE9);

    if (depthFBTex == 0)    depthFBTex  = gen2DTexture( w, h, GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_FLOAT );
    if (normalFBTex == 0)   normalFBTex = gen2DTexture( w, h, GL_RGBA32F, GL_RGBA, GL_FLOAT );
    if (positionFBTex == 0) positionFBTex = gen2DTexture( w, h, GL_RGBA32F, GL_RGBA, GL_FLOAT );
    if (colorFBTex == 0) colorFBTex    = gen2DTexture( w, h, GL_RGBA32F, GL_RGBA, GL_FLOAT );

    if (FBO[0] == 0) glGenFramebuffers(1, &FBO[0]);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO[0]);

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
    
    FBOstatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(FBOstatus != GL_FRAMEBUFFER_COMPLETE)
	{
        cout << " - GL_FRAMEBUFFER_COMPLETE failed, CANNOT use FBO[0]\n" << endl;
        checkFramebufferStatus(FBOstatus);
    }
}

static void initShadowMapFBO(int w, int h)
{
    if (shadowmapTex == 0)
    {
        glGenTextures(1, &shadowmapTex );
        glBindTexture(GL_TEXTURE_2D, shadowmapTex );
        glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32 , 1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0 );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    }
    
    if (FBO[1] == 0) glGenFramebuffers(1, &FBO[1]);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO[1]);
    glFramebufferTexture( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowmapTex, 0 );
    glDrawBuffer( GL_NONE );

    GLenum FBOstatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(FBOstatus != GL_FRAMEBUFFER_COMPLETE)
	{
        cout << " - GL_FRAMEBUFFER_COMPLETE failed, CANNOT use FBO[1]\n" << endl;
        checkFramebufferStatus(FBOstatus);
    }

}

////////////////////////////////////////////////////////////////
//// Public interface
//// Create application FB 
////////////////////////////////////////////////////////////////
void initFBO( int w, int h )
{
    initGeometryMapFBO(w, h);
    initShadowMapFBO(1024, 1024);
}

void freeFBO() 
{
    if (depthFBTex) glDeleteTextures(1,&depthFBTex);
    if (normalFBTex) glDeleteTextures(1,&normalFBTex);
    if (positionFBTex) glDeleteTextures(1,&positionFBTex);
    if (colorFBTex) glDeleteTextures(1,&colorFBTex);
    glDeleteFramebuffers(1,&FBO[0]);
    glDeleteFramebuffers(1,&FBO[1]);
    FBO[0] = FBO[1] = 0;
    depthFBTex = normalFBTex = positionFBTex = colorFBTex = 0;
}



static void CreateScreenQuadData()
{
    //GLenum err;
    vertex2_t verts [] =
    {
		{vec3(-1, 1,0),vec2(0,1)},
        {vec3(-1,-1,0),vec2(0,0)},
        {vec3( 1,-1,0),vec2(1,0)},
        {vec3( 1, 1,0),vec2(1,1)}
    };

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
    glBindBuffer(GL_ARRAY_BUFFER, 0 );
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0 );
}

void CreateModelData()
{
    int numModel = g_meshloader.getModelCount();
    if (numModel >= QUAD)
    {
        cout << "- More Models than expected, cannot handle, exit!" << endl;
        exit(1);
    }
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
        else
        {
            printf("- Model %d, Has no Normal.\n", i);
        }
        if( model->numTxcoord > 0 )
        {
            glGenBuffers( 1, &tbo[i] );
            glBindBuffer( GL_ARRAY_BUFFER, tbo[i] );
            glBufferData( GL_ARRAY_BUFFER, sizeof(float) * 2 * model->numVert, model->tbo, GL_STATIC_DRAW  );

            glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, 0, 0 );
            glEnableVertexAttribArray( 2 );
        }
        else
        {
            printf("- Model %d, Has no Texture coord.\n", i);
        }

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

        glGenBuffers( 1, &ibo[i] );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo[i] );
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( unsigned int ) * model->numIdx, model->ibo, GL_STATIC_DRAW );
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, 0 );
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0 );
    glBindVertexArray(0);
}

void initVertexData()
{
    static int count = 0;
    assert(count == 0);
    CreateModelData();
    CreateScreenQuadData();
    ++count;
}

void initShader()
{
    geometryShader.init( "shader/pass.vert.glsl", "shader/pass.frag.glsl" );
    phongShader.init( "shader/shade.vert.glsl", "shader/diagnostic.frag.glsl" );
    shadowmapShader.init( "shader/shadowmap.vert.glsl", "shader/shadowmap.frag.glsl" );
    plainShader.init("shader/plain.vert.glsl", "shader/plain.frag.glsl");
    visualNormalShader.init("shader/visualNormal.vert.glsl", "shader/visualNormal.frag.glsl", "shader/visualNormal.geom.glsl");
}

void initLight()
{
    light1.pos = light1.initialPos = vec4( 0, 0.53, 0, 1.0 );
    light1.color = vec3( 1, 1, 1 );
}