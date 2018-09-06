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
	{ vec3(-1, 1,0),vec2(0,1) },
	{ vec3(-1,-1,0),vec2(0,0) },
	{ vec3(1, -1,0),vec2(1,0) },
	{ vec3(1,  1,0),vec2(1,1) }
    };
    unsigned short indices[] = { 0,1,2,  0,2,3 }; //CCW

    glGenBuffers(1,&vbo[QUAD]);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[QUAD]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glGenVertexArrays( 1, &vao[QUAD] );
    glBindVertexArray( vao[QUAD] );
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

void Renderer::CreateModelBuffers()
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
        glGenBuffers( 1, &vbo[i] );
        glBindBuffer( GL_ARRAY_BUFFER, vbo[i] );
        glBufferData( GL_ARRAY_BUFFER, sizeof(float) * 3 * model->numVert, model->vbo, GL_STATIC_DRAW  );

        glGenVertexArrays( 1, &vao[i] );
        glBindVertexArray( vao[i] );

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


void Renderer::CreateShadowFB(int w, int h)
{
    if (shadowmapTex == 0)
    {
        glGenTextures(1, &shadowmapTex);
        glBindTexture(GL_TEXTURE_2D, shadowmapTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    if (FBO[1] != 0)
        glDeleteFramebuffers(1, &FBO[1]);
    glGenFramebuffers(1, &FBO[1]);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO[1]);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowmapTex, 0);
    glDrawBuffer(GL_NONE);

    GLenum FBOstatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (FBOstatus != GL_FRAMEBUFFER_COMPLETE)
    {
        cout << " - GL_FRAMEBUFFER_COMPLETE failed, CANNOT use Shadow FB, FBO[1]\n" << endl;
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

void Renderer::CreateGeomFB(int w, int h)
{
    GLenum FBOstatus;

	glEnable(GL_TEXTURE_2D);
    //glActiveTexture(GL_TEXTURE9);

    if (depthFBTex)
        glDeleteTextures(1, &depthFBTex);
    depthFBTex = gen2DTexture(w, h, GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_FLOAT);

    if (normalFBTex)
        glDeleteTextures(1, &normalFBTex);
    normalFBTex = gen2DTexture(w, h, GL_RGBA32F, GL_RGBA, GL_FLOAT);

    if (positionFBTex)
        glDeleteTextures(1, &positionFBTex);
    positionFBTex = gen2DTexture(w, h, GL_RGBA32F, GL_RGBA, GL_FLOAT);

    if (colorFBTex)
        glDeleteTextures(1, &colorFBTex);
    colorFBTex = gen2DTexture(w, h, GL_RGBA32F, GL_RGBA, GL_FLOAT);

    glGenFramebuffers(1, &FBO[0]);
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

void Renderer::DrawGeometryPass()
{
	RenderOption & option = RenderOption::getRenderOption();
	mat4 world(1.f);
	option.world = world;

    ShaderManager& shdmgr = ShaderManager::getShaderManager();
    ShaderProgram& shader = shdmgr.ActiveShader("GeometryPass");
    shdmgr.UpdateShaderParam("GeometryPass");

	glEnable(GL_TEXTURE_2D);

	int bTextured;
	int numModel = mModels->getModelCount();
	for (int j = 0; j < numModel; ++j)
	{
		const ObjModel* model = g_meshloader.getModel(j);
		glBindVertexArray(vao[j]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo[j]);
		for (int i = 0; i < model->numGroup; ++i)
		{
			model->groups[i].shininess = 50;
			shader.setParameter(shader::fv3, &model->groups[i].kd, "u_Color");
			shader.setParameter(shader::f1, &model->groups[i].shininess, "u_Shininess");
			if (model->groups[i].texId > 0)
			{
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, model->groups[i].texId);
				shader.setTexParameter(0, "u_ColorTex");
				bTextured = 1;
			}
			else
				bTextured = 0;
			shader.setParameter(shader::i1, &bTextured, "u_bTextured");

			if (model->groups[i].bumpTexId > 0)
			{
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, model->groups[i].bumpTexId);
				shader.setTexParameter(1, "u_BumpTex");
				bTextured = 1;
			}
			else
				bTextured = 0;
			shader.setParameter(shader::i1, &bTextured, "u_bBump");

			glDrawElements(GL_TRIANGLES, 3 * model->groups[i].numTri, GL_UNSIGNED_INT, (void*)model->groups[i].ibo_offset);
		}
	}
}

void Renderer::UpdateCamera(void)
{
	static float theta = 0.f;
	vec3 eye(0.f, .6f, 2.f);
	vec3 at(0.f, 0.f, 0.f);
	float d = glm::length(eye - at);
	theta += 0.001 * 3.1415926f;
	float x = d * cos(theta);
	float z = d * sin(theta);
	eye.x = x;
	eye.z = z;

	RenderOption& option = RenderOption::getRenderOption();
	option.view = glm::lookAt(eye, at, vec3(0.f, 1.f, 0.f));
	option.proj = glm::perspective(90.f, 16.f / 9.f, 0.1f, 100.f);
	option.eye = eye;
}

void Renderer::DrawLight()
{
	mat4 lightWorld = glm::translate(1.f, 1.f, 0.f);
	lightWorld = glm::scale(lightWorld, 0.02f, 0.02f, 0.02f);
	lightWorld = glm::rotate(lightWorld, 90.f, 1.f, 0.f, 0.f);

	vec4 lightPos = lightWorld * vec4(0.f, 0.f, 0.f, 1.f);
	
	// error
	//vec4 lightPos = vec4(0.f, 0.f, 0.f, 1.f) * lightWorld;

	RenderOption & option = RenderOption::getRenderOption();
	option.world = lightWorld;
	option.lightIntensity = vec3(1.f, 0.1f, 0.9f) * 1.f;
	option.lightPosition = lightPos.xyz;

	ShaderManager& shdmgr = ShaderManager::getShaderManager();
	ShaderProgram& shader = shdmgr.ActiveShader("Light");
	shdmgr.UpdateShaderParam("Light");

	glBindVertexArray(vao[QUAD]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[QUAD]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo[QUAD]);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
}

void Renderer::DrawPlane()
{
	mat4 lightWorld = glm::translate(0.f, -1.f, -2.f);
	lightWorld = glm::scale(lightWorld, 3.f, 3.f, 3.f);
	lightWorld = glm::rotate(lightWorld, 90.f, 1.f, 0.f, 0.f);

	RenderOption & option = RenderOption::getRenderOption();
	option.world = lightWorld;
	option.lightIntensity = vec3(0.3f, 0.3f, 0.3f);

	ShaderManager& shdmgr = ShaderManager::getShaderManager();
	ShaderProgram& shader = shdmgr.ActiveShader("Light");
	shdmgr.UpdateShaderParam("Light");

	glBindVertexArray(vao[QUAD]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[QUAD]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo[QUAD]);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
}

void Renderer::DrawScreenPass()
{
	ShaderManager& shdmgr = ShaderManager::getShaderManager();
	ShaderProgram& shader = shdmgr.ActiveShader("FinalPass");
	shdmgr.UpdateShaderParam("FinalPass");

	glEnable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthFBTex);
	shader.setTexParameter(0, "u_Depthtex");

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normalFBTex);
	shader.setTexParameter(1, "u_Normaltex");

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, positionFBTex);
	shader.setTexParameter(2, "u_Positiontex");

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, colorFBTex);
	shader.setTexParameter(3, "u_Colortex");

#if 0
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, shadowmapTex);
	shader.setTexParameter(4, "u_shadowmap");
#endif
	//Draw the screen space quad
	glBindVertexArray(vao[QUAD]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[QUAD]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo[QUAD]);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
}

void Renderer::RenderShadowPass()
{

}

void Renderer::DrawDrawCamera(std::string shadernam, std::string cameraname)
{

}

void Renderer::DrawModel(std::string shadername)
{
	RenderOption & option = RenderOption::getRenderOption();
	mat4 world(1.f);
	option.world = world;

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

void Renderer::RenderForward()
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
    //DrawModel("Phong");
    glDisable( GL_CULL_FACE );
	DrawLight();
	DrawPlane();
    if (option.drawVnormal)
    {
        DrawModel("VisualNormal");
    }
}

void Renderer::RenderDeferred()
{
    ViewPort sceneViewPort(0, 0, mBackBufferWidth, mBackBufferHeight);
    sceneViewPort.apply();

	RenderOption &option = RenderOption::getRenderOption();

	//first pass
	glBindFramebuffer(GL_FRAMEBUFFER, FBO[0]);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	DrawGeometryPass();

	//second pass
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear( GL_COLOR_BUFFER_BIT );
	glDisable(GL_DEPTH_TEST);
	DrawScreenPass();

	//third pass
	if (option.drawVnormal)
	{
		DrawModel("VisualNormal");
	}
}

