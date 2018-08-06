#include <gl/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include "shader.h"
#include "ShaderManager.h"
#include "camera.h"
#include "Renderer.h"

namespace shader
{

using namespace glm;

static ShaderManager shadermgr;
ShaderManager& ShaderManager::getShaderManager()
{
    return shadermgr;
}

ShaderManager::ShaderManager()
{
    //BuildShaderList();
}

void ShaderManager::BuildShaderList()
{
    mShaderList["VisualNormal"] = new ShaderProgram("shader/visualNormal.vert.glsl", "shader/visualNormal.frag.glsl", "shader/visualNormal.geom.glsl");
    mShaderList["Plain"]        = new ShaderProgram("shader/plain.vert.glsl", "shader/plain.frag.glsl");
}


void ShaderManager::ActiveShader(const std::string& name)
{
    ShaderProgram & shader = *mShaderList[name];
    shader.use();
}

void ShaderManager::UpdateShaderParam(const std::string& name)
{
    ShaderProgram & shader = *mShaderList[name];
    mat4 world(1.0f);
    RenderOption & option = RenderOption::getRenderOption();
    if (name == "Observer")
    {
        Camera &camera = CameraManager::getCamera("observe");
        mat4 model, view, projection, normalToView, normalToWorld;
        camera.Update();
        camera.GetMatricies(projection, view, model);
        normalToView  = transpose(inverse(view * world));
        normalToWorld = transpose(inverse(world));

        shader.setParameter(shader::mat4x4, (void*)&world[0][0], "u_World");
        shader.setParameter(shader::mat4x4, (void*)&view[0][0], "u_View");
        shader.setParameter(shader::mat4x4, (void*)&projection[0][0], "u_Projection");
        shader.setParameter(shader::mat4x4, (void*)&normalToWorld[0][0], "u_NormalToWorld");
        shader.setParameter(shader::mat4x4, (void*)&normalToView[0][0],  "u_NormalToView");
    }
    else 
    {
        Camera &camera = CameraManager::getCamera("draw");
        mat4 model, view, projection, normalToView, normalToWorld;
        camera.Update();
        camera.GetMatricies(projection, view, model);
        normalToView  = transpose(inverse(view * world));
        normalToWorld = transpose(inverse(world));

        shader.setParameter(shader::mat4x4, (void*)&world[0][0], "u_World");
        shader.setParameter(shader::mat4x4, (void*)&view[0][0], "u_View");
        shader.setParameter(shader::mat4x4, (void*)&projection[0][0], "u_Projection");
        shader.setParameter(shader::mat4x4, (void*)&normalToWorld[0][0], "u_NormalToWorld");
        shader.setParameter(shader::mat4x4, (void*)&normalToView[0][0],  "u_NormalToView");
    }

    if (name == "VisualNormal")
    {
        const float normalStrength = .05f;
        shader.setParameter(shader::f1, (void*)&normalStrength, "u_NormalLength");
    }
    else if (name == "Plain")
    {
        const int diag = option.diagType;
        shader.setParameter(shader::i1, (void*)&diag, "u_DiagType");
    }
    else
    {

    }
}

}