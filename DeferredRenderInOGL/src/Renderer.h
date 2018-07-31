#ifndef _GLRENDERER_H
#define _GLRENDERER_H

#include <iostream>
#include <vector>
#include <gl/glew.h>
#include <gl/freeglut.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include "shader.h"
#include "objLoader.h"
#include "camera.h"
#include "util.h"
#include "ShaderManager.h"

using namespace glm;

typedef objLoader ModelContainer;

enum DebugType
{
    DEBUG_FULL_SCENE = 1,
    DEBUG_POSITION   = 2,
    DEBUG_NORMAL     = 3,
    DEBUG_COLOR      = 4,
    DEBUG_SHADOW     = 5,
};

struct QuadVertexFormat
{
	glm::vec3 pt;
	glm::vec2 texcoord;
};

struct RenderOption
{
    bool multipleView;
    bool shadowOff;
    bool wireframe;
    bool visualizeVertexNormal;
    DebugType diagType;

    RenderOption()
        : multipleView(false)
        , shadowOff(true)
        , wireframe(false)
        , visualizeVertexNormal(false)
        , diagType(DebugType::DEBUG_FULL_SCENE)
    { }

    static RenderOption& getRenderOption();
};

class Renderer
{

private:
    mat4 mWorld;
    mat4 mView;
    mat4 mProjection;
    mat4 mNormalToView;
    mat4 mNormalToWorld;
    mat4 mLightMVP;

    ////////////////////////////////////////////////////////////////
    ////OpenGL buffer objects for loaded mesh
    ////////////////////////////////////////////////////////////////
    GLuint vao[10] = { 0 };
    GLuint vbo[10] = { 0 };
    GLuint nbo[10] = { 0 };
    GLuint tbo[10] = { 0 };
    GLuint ibo[10] = { 0 };

    const int QUAD = 8;

    ////////////////////////////////////////////////////////////////
    ////FB Textures for geom data 
    ////////////////////////////////////////////////////////////////
    GLuint positionFBTex = 0;
    GLuint normalFBTex = 0;
    GLuint colorFBTex = 0;
    GLuint depthFBTex = 0;
    GLuint shadowmapTex = 0;

    ////////////////////////////////////////////////////////////////
    ////Framebuffer objects
    ////////////////////////////////////////////////////////////////
    enum FrameBufferType
    {
        GEOMETRY_FB = 0,
        SHADOW_FB,
        TOTAL_FB
    };

    GLuint FBO[TOTAL_FB] = { 0, 0 };

private:

    // Select object in scene, put it in new place.
    void UpdateWorldMatrix()
    {
        // How to update mWorld according to User input ?
        mNormalToWorld = transpose(inverse(mWorld));
    }

    void RenderSingleViewport();
    void RenderMultipleViewport();

    void CreatedScreenQaudBuffer();
    void CreateObjectModelBuffer();
    void CreateShadowFB();
    void CreateGeomFB();

    void RenderToGeomFB();
    void RenderToShadowFB();
    void RenderLighting();

    void RenderPlainModel();
    void RenderVertexNormal();

    ModelContainer *mModels;

public:

    void SetUp(ModelContainer* models)
    {
        mModels = models;
        CreatedScreenQaudBuffer();
        CreateObjectModelBuffer();

        CreateShadowFB();
        CreateGeomFB();
    }

    void  Render(void)
    {
        if (RenderOption::getRenderOption().multipleView)
            this->RenderMultipleViewport();
        else
            this->RenderSingleViewport();
    }

};

class RendererManager
{
public:
    static Renderer& getRender(std::string name = "");
};

#endif