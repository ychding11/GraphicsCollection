#include <gl/glew.h>
#include "shader.h"
#include "ShaderManager.h"


namespace shader
{

ShaderManager::ShaderManager()
{
    BuildShaderList();
}

void ShaderManager::BuildShaderList()
{
    mShaderList["VisualNormal"] = new ShaderProgram("shader/visualNormal.vert.glsl", "shader/visualNormal.frag.glsl", "shader/visualNormal.geom.glsl");
}

ShaderManager& ShaderManager::getShaderManager()
{

}

void ShaderManager::ActiveShader(const std::string& name)
{
    ShaderProgram & shader = *mShaderList[name];
    shader.use();
}

void ShaderManager::UpdateShaderParam(const std::string& name)
{
    ShaderProgram & shader = *mShaderList[name];
    if (name == "")
    {

    }
    else if (name == "")
    {

    }
    else
    {

    }
}

}