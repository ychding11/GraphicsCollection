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
    mShaderList["GeometryPass"] = new ShaderProgram("shader/geometryPass.vert", "shader/geometryPass.frag");
    mShaderList["FinalPass"]    = new ShaderProgram("shader/finalPass.vert", "shader/finalPass.frag");
    mShaderList["Phong"]        = new ShaderProgram("shader/phong.vert.glsl", "shader/phong.frag.glsl");
    mShaderList["Light"]        = new ShaderProgram("shader/light.vert", "shader/light.frag");
}


ShaderProgram& ShaderManager::ActiveShader(const std::string& name)
{
    ShaderProgram & shader = *mShaderList[name];
    shader.use();
	return shader;
}

void ShaderManager::UpdateShaderParam(const std::string& name)
{
    ShaderProgram & shader = *mShaderList[name];
    mat4 world(1.0f);
    RenderOption & option = RenderOption::getRenderOption();
	vec3 eye(0.f,.0f,2.f);

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
#if 1
        Camera &camera = CameraManager::getCamera("draw");
        mat4 model, view, projection, normalToView, normalToWorld;
        camera.Update();
        camera.GetMatricies(projection, view, model);

		static float theta = 0.f;
		vec3 at(0.f, 0.f, 0.f);
		float d = glm::length(eye - at);
		theta += 0.001 * 3.1415926f;
		float x = d * cos(theta);
		float z = d * sin(theta);
		eye.x = x;
		eye.z = z;

		view = glm::lookAt(eye, at, vec3(0.f, 1.f, 0.f));
		projection = glm::perspective(90.f, 16.f/9.f, 0.1f, 100.f);

        normalToView  = transpose(inverse(view * world));
        normalToWorld = transpose(inverse(world));

        shader.setParameter(shader::mat4x4, (void*)&world[0][0], "u_World");
        shader.setParameter(shader::mat4x4, (void*)&view[0][0], "u_View");
        shader.setParameter(shader::mat4x4, (void*)&projection[0][0], "u_Projection");
        shader.setParameter(shader::mat4x4, (void*)&normalToWorld[0][0], "u_NormalToWorld");
        shader.setParameter(shader::mat4x4, (void*)&normalToView[0][0],  "u_NormalToView");
#endif
    }

    if (name == "VisualNormal")
    {
        const float normalStrength = .05f;
        shader.setParameter(shader::f1, (void*)&normalStrength, "u_NormalLength");
    }
    else if (name == "Light")
    {
#if 0
		mat4 lightWorld(0.1f, 0.0f, 0.0f, 0.f,
						0.0f, 0.1f, 0.0f, 0.f,
						0.0f, 0.0f, 0.1f, 0.f,
						0.0f, 1.0f, 0.0f,  1.f );
#endif
		mat4 lightWorld = glm::translate(0.f, 1.f, 0.f);
		lightWorld = glm::scale(lightWorld, 0.3f, 0.3f, 0.3f);
		lightWorld = glm::rotate(lightWorld, 90.f, 1.f, 0.f, 0.f);

		vec3 lightIntensity(1.f, 1.f, 1.f);
		shader.setParameter(shader::mat4x4, (void*)&lightWorld[0][0], "u_World");
		shader.setParameter(shader::fv3, (void*)&lightIntensity, "u_LightIntensity");
    }
    else if (name == "Plain")
    {
        const int diag = option.diagType;
        shader.setParameter(shader::i1, (void*)&diag, "u_DiagType");
    }
    else if (name == "Phong")
    {
        shader.setParameter(shader::fv3, (void*)&eye, "u_EyePos");
    }
    else if (name == "GeometryPass")
    {
    }
    else if (name == "FinalPass")
    {
#if 1
        const int diag = option.diagType;
		const int shadowOn = !option.shadowOff;
        shader.setParameter(shader::i1, (void*)&diag, "u_DiagType");
        shader.setParameter(shader::i1, (void*)&shadowOn, "u_ShadowOn");

		glm::vec3 lightPos(0.f, 6.f, 0.f), lightColor(1.f, 1.f, 1.f);
		glm::mat4 biasLightMVP(1.f);
		shader.setParameter(shader::fv4, &lightPos[0], "u_LightPos");
		shader.setParameter(shader::fv3, &lightColor[0], "u_LightColor");
		shader.setParameter(shader::mat4x4, &biasLightMVP[0][0], "u_LightMVP");

		Camera &camera = CameraManager::getCamera("draw");
		shader.setParameter(shader::fv3, &camera.camera_position[0], "u_eyePos");
#endif
    }
    else
    {

    }
}

}