#include "glslUtility.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace std;

namespace glslUtility
{

	char* loadFile(const char *fname, GLint &fSize)
	{
		ifstream::pos_type size;
		char * memblock;
		std::string text;

		// file read based on example in cplusplus.com tutorial
		ifstream file (fname, ios::in|ios::binary|ios::ate);
		if (file.is_open())
		{
			size = file.tellg();
			fSize = (GLuint) size;
			memblock = new char [size];
			file.seekg (0, ios::beg);
			file.read (memblock, size);
			file.close();
			cout << "file " << fname << " loaded" << endl;
			text.assign(memblock);
		}
		else
		{
			cout << "Unable to open file " << fname << endl;
			exit(1);
		}
		return memblock;
	}

	void printShaderInfoLog(GLint shader)
	{
		int infoLogLen = 0;
		int charsWritten = 0;

		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);
		if (infoLogLen > 0)
		{
			GLchar* infoLog = new GLchar[infoLogLen + 1];
			glGetShaderInfoLog(shader,infoLogLen, &charsWritten, infoLog);
			cout << "\n- Shader Compile Log: \t" << endl << infoLog << endl;
			cout << "---------------------------------------------" << endl;
			delete [] infoLog;
		}
	}

	void printLinkInfoLog(GLint prog) 
	{
		int infoLogLen = 0;
		int charsWritten = 0;
		GLchar *infoLog;

		glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &infoLogLen);
		if (infoLogLen > 0)
		{
			infoLog = new GLchar[infoLogLen + 1];
			glGetProgramInfoLog(prog,infoLogLen, &charsWritten, infoLog);
			cout << "\n- Shader Link Log: \t" << endl << infoLog << endl;
			cout << "------------------------------------------" << endl;
			delete [] infoLog;
		}
	}

    GLuint initshaders (GLenum type, const char *filename) 
    {
        GLuint shader = glCreateShader(type) ; 
        GLint compiled ; 
        char *ss;
        GLint slen;

        ss = loadFile(filename,slen);
        const char * cs = ss;

        glShaderSource (shader, 1, &cs, &slen) ; 
        glCompileShader (shader) ; 
        glGetShaderiv (shader, GL_COMPILE_STATUS, &compiled) ; 
		if (!compiled)
		{
			cout << "Shader compile failed: " << filename << endl;
			printShaderInfoLog(shader);
		} 
        delete [] ss;

        return shader ; 
    }

	shaders_t loadShaders(const char * vert_path, const char * frag_path, const char * geom_path, const char * compute_path)
    {
		GLuint f = 0, v = 0, g = 0, c = 0;
        if( vert_path )
		    v = initshaders( GL_VERTEX_SHADER, vert_path );
        if( frag_path )
            f = initshaders( GL_FRAGMENT_SHADER, frag_path );
        if( geom_path )
            g = initshaders( GL_GEOMETRY_SHADER, geom_path );
        if( compute_path )
            c = initshaders( GL_COMPUTE_SHADER, compute_path );

        shaders_t out;
        out.vertex = v; out.fragment = f; out.geometry = g; out.compute = c;

		return out;
	}

	void attachAndLinkProgram( GLuint program, shaders_t shaders) 
    {
        if( shaders.vertex )
            glAttachShader(program, shaders.vertex);
        if( shaders.fragment )
		    glAttachShader(program, shaders.fragment);
        if( shaders.geometry )
            glAttachShader(program, shaders.geometry  );
        if( shaders.compute)
            glAttachShader(program, shaders.compute  );

		GLint linked;
		glLinkProgram(program);
		glGetProgramiv(program,GL_LINK_STATUS, &linked);
		if (!linked) 
		{
			cout << "Program did not link." << endl;
			printLinkInfoLog(program);
		}
	}
}