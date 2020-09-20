#version 430

uniform mat4x4 u_Model;
uniform mat4x4 u_View;
uniform mat4x4 u_Persp;

//uniform vec4 kd;

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 texcoord;

out vec3 fs_Normal;
out vec4 fs_Position;
out vec2 fs_texcoord;

void main(void)
{
    vec4 worldPos = u_Model * vec4(Position, 1.0);
    vec4 viewPos  = u_View  * worldPos;
    gl_Position   = u_Persp * viewPos;

    fs_Normal   = Normal;
    fs_Position = worldPos;
	fs_texcoord = texcoord;
}