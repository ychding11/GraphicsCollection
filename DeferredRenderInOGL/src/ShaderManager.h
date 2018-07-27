#ifndef _SHADER_MANAGER_H
#define _SHADER_MANAGER_H

#include <map>
#include <vector>
#include  "shader.h"

namespace shader
{

class ShaderManager
{
private:

    std::map<std::string, ShaderProgram*> mShaderList;
    void BuildShaderList();

public:
    ShaderManager();
    static ShaderManager& getShaderManager();

    void ActiveShader(const std::string& name);
    void UpdateShaderParam(const std::string& name);

};

}
#endif