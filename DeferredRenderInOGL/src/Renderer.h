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
    DEBUG_FULL_SCENE = 0,
    DEBUG_POSITION   = 1,
    DEBUG_NORMAL     = 2,
    DEBUG_COLOR      = 3,
    DEBUG_SHADOW     = 4,
    DEBUG_NUM= 5,
};

struct QuadVertexFormat
{
	glm::vec3 pt;
	glm::vec2 texcoord;
};

struct ViewPort
{
    int x, y;
    int w, h;

    ViewPort(int xx = 0, int yy = 0, int ww = 0, int hh = 0)
        : x(xx), y(yy)
        , w(ww), h(hh)
    {}

    void apply()
    {
        glViewport(x, y, w, h);
    }

};

struct RenderOption
{
    bool deferred;
    bool shadowOff;
    bool wireframe;
    bool drawVnormal;
    DebugType diagType;
	vec3 lightPosition;
	vec3 lightIntensity;
	vec3 eye;
	mat4 world;
	mat4 view;
	mat4 proj;

    RenderOption()
        : deferred(false)
        , shadowOff(true)
        , wireframe(false)
        , drawVnormal(false)
        , diagType(DebugType::DEBUG_FULL_SCENE)
		, lightPosition(0.f)
		, lightIntensity(1.f)
		, eye(0.f)
		, world(1.f)
		, view(1.f)
		, proj(1.f)
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

    void RenderForward();
    void RenderDeferred();

    void CreatedScreenQaudBuffer();

	//Create vao vbo ibo for model
    void CreateModelBuffers();

    void CreateShadowFB(int w = 1024, int h = 1024);

	//Create Frame Buffers for Deferred Shading
    void CreateGeomFB(int w, int h);

	// render geometry data into FB
    void DrawGeometryPass();

	// render lighting to screen
    void DrawScreenPass();

    void RenderShadowPass();

	//Draw Model with different shaders
    void DrawModel(std::string shadername);

	//Draw light source
	void DrawLight();

	//Draw Plane 
	void DrawPlane();

	void UpdateCamera(void);
    void DrawDrawCamera(std::string shadernam, std::string cameraname);

    void SplitBackBuffer(ViewPort &vp1, ViewPort &vp2);
    void ObserveScene();

    ModelContainer *mModels;

    int mBackBufferHeight;
    int mBackBufferWidth;

public:

    void UpdateBackBuffer(int width, int height)
    {
        mBackBufferHeight = height;
        mBackBufferWidth  = width;

        //Handle resource reclaim
        CreateGeomFB(width, height);
    }

    void SetUp(ModelContainer* models)
    {
        mModels = models;
        CreatedScreenQaudBuffer();
        CreateModelBuffers();

        CreateShadowFB();
    }

    void  Render(void)
    {
		UpdateCamera();
        if (RenderOption::getRenderOption().deferred)
            this->RenderDeferred();
        else
            this->RenderForward();
    }

};

class RendererManager
{
public:
    static Renderer& getRender(std::string name = "");
};

#endif