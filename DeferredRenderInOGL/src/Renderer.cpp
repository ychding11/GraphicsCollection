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
#include "Renderer.h"

using namespace std;
using namespace glm;
using namespace shader;

static Renderer render;

Renderer& RendererManager::getRender(std::string name)
{
    return render;
}

static RenderOption renderOption;
RenderOption & RenderOption::getRenderOption()
{
    return renderOption;
}

////////////////////////////////////////////////////////
////
////////////////////////////////////////////////////////


void Renderer::CreatedScreenQaudBuffer()
{
    QuadVertexFormat verts [] =
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

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QuadVertexFormat), 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertexFormat), (void*)sizeof(vec3));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glGenBuffers(1,&ibo[QUAD]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo[QUAD]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6*sizeof(GLushort), indices, GL_STATIC_DRAW);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0 );
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0 );
}

void Renderer::CreateObjectModelBuffer()
{
    int numModel = mModels->getModelCount();
    if (numModel >= QUAD)
    {
        cout << "- More Models than expected, cannot handle, exit!" << endl;
        exit(1);
    }
    for( int i = 0; i < numModel; ++i )
    {
        const ObjModel* model = mModels->getModel(i);
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
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * model->numIdx, model->ibo, GL_STATIC_DRAW );
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, 0 );
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0 );
    glBindVertexArray(0);
}


void Renderer::CreateShadowFB()
{
    if (shadowmapTex == 0)
    {
        glGenTextures(1, &shadowmapTex);
        glBindTexture(GL_TEXTURE_2D, shadowmapTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, 1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    if (FBO[1] == 0) glGenFramebuffers(1, &FBO[1]);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO[1]);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowmapTex, 0);
    glDrawBuffer(GL_NONE);

    GLenum FBOstatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (FBOstatus != GL_FRAMEBUFFER_COMPLETE)
    {
        cout << " - GL_FRAMEBUFFER_COMPLETE failed, CANNOT use FBO[1]\n" << endl;
        checkFramebufferStatus(FBOstatus);
    }
}

static GLuint  gen2DTexture(int w, int h, GLenum internalFormat, GLenum format, GLenum type)
{
    GLuint texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, format, type, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texId;
}

void Renderer::CreateGeomFB()
{
    GLenum FBOstatus;
    int w = mBackBufferWidth;
    int h = mBackBufferHeight;

    glActiveTexture(GL_TEXTURE9);

    if (depthFBTex == 0)    depthFBTex = gen2DTexture(w, h, GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_FLOAT);
    if (normalFBTex == 0)   normalFBTex = gen2DTexture(w, h, GL_RGBA32F, GL_RGBA, GL_FLOAT);
    if (positionFBTex == 0) positionFBTex = gen2DTexture(w, h, GL_RGBA32F, GL_RGBA, GL_FLOAT);
    if (colorFBTex == 0)    colorFBTex = gen2DTexture(w, h, GL_RGBA32F, GL_RGBA, GL_FLOAT);

    if (FBO[0] == 0) glGenFramebuffers(1, &FBO[0]);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO[0]);

    glReadBuffer(GL_NONE);

    GLenum draws[3];
    draws[0] = GL_COLOR_ATTACHMENT0;
    draws[1] = GL_COLOR_ATTACHMENT1;
    draws[2] = GL_COLOR_ATTACHMENT2;

    glDrawBuffers(3, draws);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthFBTex, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, draws[0], normalFBTex, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, draws[1], positionFBTex, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, draws[2], colorFBTex, 0);

    FBOstatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (FBOstatus != GL_FRAMEBUFFER_COMPLETE)
    {
        cout << " - GL_FRAMEBUFFER_COMPLETE failed, CANNOT use FBO[0]\n" << endl;
        checkFramebufferStatus(FBOstatus);
    }
}

void Renderer::RenderToGeomFB()
{

}

void Renderer::RenderToShadowFB()
{

}

void Renderer::RenderLighting()
{

}

void Renderer::DrawDrawCamera(std::string shadernam, std::string cameraname)
{

}

void Renderer::DrawModel(std::string shadername)
{
    ShaderManager& shdmgr = ShaderManager::getShaderManager();
    shdmgr.ActiveShader(shadername);
    shdmgr.UpdateShaderParam(shadername);

    int numModel = mModels->getModelCount();
    for( int i = 0; i < numModel; ++i )
    {
        glBindVertexArray( vao[i] );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo[i] );
        const ObjModel* model = mModels->getModel(i);
        for( int i = 0; i < model->numGroup; ++i )
        {
            glDrawElements( GL_TRIANGLES, 3 * model->groups[i].numTri , GL_UNSIGNED_INT, (void*)model->groups[i].ibo_offset );
        }
    }
}

void Renderer::DrawVertexNormal(std::string shadername)
{
    ShaderManager& shdmgr = ShaderManager::getShaderManager();
    shdmgr.ActiveShader(shadername);
    shdmgr.UpdateShaderParam(shadername);

    int numModel = mModels->getModelCount();
    for (int i = 0; i < numModel; ++i)
    {
        glBindVertexArray(vao[i]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo[i]);
        const ObjModel* model = mModels->getModel(i);
        for (int i = 0; i < model->numGroup; ++i)
        {
            glDrawElements(GL_TRIANGLES, 3 * model->groups[i].numTri, GL_UNSIGNED_INT, (void*)model->groups[i].ibo_offset);
        }
    }
}

void Renderer::SplitBackBuffer(ViewPort &vp1, ViewPort &vp2)
{
    int h1 = mBackBufferHeight / 2.f;
    int h2 = mBackBufferHeight - h1 - 5;
    vp1.w = mBackBufferWidth;
    vp1.h = h1;

    vp2.x = 0;
    vp2.y = h1 + 5;
    vp2.w = mBackBufferWidth;
    vp2.h = h2;
}

void Renderer::ObserveScene()
{
    ViewPort sceneViewPort(0, 0, mBackBufferWidth, mBackBufferHeight);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );

    RenderOption &option = RenderOption::getRenderOption();
    
    sceneViewPort.apply();
    DrawModel("Plain");
    //DrawCamera();
}

void Renderer::RenderSingle()
{
    ViewPort sceneViewPort(0, 0, mBackBufferWidth, mBackBufferHeight);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );

    RenderOption &option = RenderOption::getRenderOption();
    if (option.wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    sceneViewPort.apply();
    DrawModel("Plain");
    if (option.drawVnormal)
    {
        DrawModel("VisualNormal");
    }
}

void Renderer::RenderMultiple()
{
    ViewPort vp1;
    ViewPort vp2;
    SplitBackBuffer(vp1, vp2);

    vp2.apply();
    ObserveScene();

    vp1.apply();
    RenderVertexNormal();
}
void Renderer::RenderVertexNormal()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );


    if (RenderOption::getRenderOption().wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    ShaderManager& shdmgr = ShaderManager::getShaderManager();
    shdmgr.ActiveShader("VisualNormal");
    shdmgr.UpdateShaderParam("VisualNormal");

    int numModel = mModels->getModelCount();
    for( int i = 0; i < numModel; ++i )
    {
        glBindVertexArray( vao[i] );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo[i] );
        const ObjModel* model = mModels->getModel(i);
        for( int i = 0; i < model->numGroup; ++i )
        {
            glDrawElements( GL_TRIANGLES, 3 * model->groups[i].numTri , GL_UNSIGNED_INT, (void*)model->groups[i].ibo_offset );
        }
    }

    shdmgr.ActiveShader("Plain");
    shdmgr.UpdateShaderParam("Plain");
    for( int i = 0; i < numModel; ++i )
    {
        glBindVertexArray( vao[i] );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo[i] );
        const ObjModel* model = mModels->getModel(i);
        for( int i = 0; i < model->numGroup; ++i )
        {
            glDrawElements( GL_TRIANGLES, 3 * model->groups[i].numTri , GL_UNSIGNED_INT, (void*)model->groups[i].ibo_offset );
        }
    }
}