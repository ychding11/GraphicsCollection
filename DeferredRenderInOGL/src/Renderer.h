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

using namespace glm;

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

enum DebugType
{
    RENDER_FULL_SCENE = 1,
    RENDER_POSITION   = 2,
    RENDER_NORMAL     = 3,
    RENDER_COLOR      = 4,
    RENDER_SHADOW     = 5,
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
        , diagType(DebugType::RENDER_FULL_SCENE)
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

    Camera& mDrawCamera;
    Camera& mObserveCamera;

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

    void UpdateCameraMatrix(Camera& camera)
    {
        mat4 model;
        camera.Update();
        camera.GetMatricies(mProjection, mView, model);
        mNormalToView = transpose(inverse(mView * mWorld));
    }

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

public:

    void SetObjectModel()
    {

    }

    
    void  Render(void)
    {
        if (RenderOption::getRenderOption().multipleView)
            this->RenderMultipleViewport();
        else
            this->RenderSingleViewport();
    }

};


#endif