#version 430

uniform mat4x4 u_World;
uniform mat4x4 u_View;
uniform mat4x4 u_Projection;
uniform mat4x4 u_NormalToWorld;
uniform mat4x4 u_NormalToView;

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 texcoord;

out Vertex
{
vec4 normal;
vec4 color;
} vertexData;

void main(void)
{
    gl_Position   = vec4(Position, 1.0);
    vertexData.normal = vec4(Normal, 0.0);
    vertexData.color  = vec4(1.f,0.f,0.f,1.f);
}