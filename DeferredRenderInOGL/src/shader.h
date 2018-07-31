#ifndef _SHADER_H
#define _SHADER_H

#include <string>

namespace shader
{

enum shaderAttrib
{
    i1, fv3, fv4, f1, mat4x4, mat3x3, tex, img
};

class ShaderProgram
{
public:

    ShaderProgram();
    ShaderProgram(const char* vs_source, const char* fs_source, const char* gs_source = 0);
    virtual ~ShaderProgram();

    int init( const char* vs_source, const char* fs_source, const char* gs_source = 0 );
    void use();
    void unuse();
    void setParameter( shaderAttrib type, void* param, char* name );
    void setTexParameter( int idx, char* name );
    void bindAttribLocation( unsigned int idx, char* name );
    void bindFragDataLocation( unsigned int idx, char* name );

protected:

    GLuint vs; //vertex shader
    GLuint fs; //fragment shader
    GLuint gs; //geometry shader
    GLuint program;

    std::string vsName;
    std::string fsName;
    std::string gsName;

};

class ComputeShader: public ShaderProgram
{
public:
    ComputeShader();
    ~ComputeShader();
    int init( const char* cs_source );
};

}
#endif