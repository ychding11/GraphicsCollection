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


static RenderOption renderOption;
RenderOption & RenderOption::getRenderOption()
{
    return renderOption;
}

////////////////////////////////////////////////////////
////
////////////////////////////////////////////////////////
void Renderer::RenderSingleViewport()
{

}

void Renderer::RenderMultipleViewport()
{

}

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
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * model->numIdx, model->ibo, GL_STATIC_DRAW );
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, 0 );
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0 );
    glBindVertexArray(0);
}

void Renderer::CreateShadowFB()
{
}

void Renderer::CreateGeomFB()
{

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

void Renderer::RenderPlainModel()
{

    }
void Renderer::RenderVertexNormal()
{
    const float normalStrength = .05f;
    //glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );

    if (RenderOption::getRenderOption().wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Update Shader Parameter
    UpdateCameraMatrix(mDrawCamera);
    visualNormalShader.use();
    visualNormalShader.setParameter( shader::f1, (void*)&normalStrength,  "u_NormalLength" );
    visualNormalShader.setParameter( shader::mat4x4, (void*)&mWorld[0][0], "u_World");
    visualNormalShader.setParameter( shader::mat4x4, (void*)&mView[0][0],  "u_View" );
    visualNormalShader.setParameter( shader::mat4x4, (void*)&mProjection[0][0], "u_Projection" );
    visualNormalShader.setParameter( shader::mat4x4, (void*)&mNormalToWorld[0][0], "u_NormalToWorld" );
    visualNormalShader.setParameter( shader::mat4x4, (void*)&mNormalToView[0][0],  "u_NormalToView" );

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
}
