#version 430

uniform mat4x4 u_World;
uniform mat4x4 u_View;
uniform mat4x4 u_Projection;
uniform mat4x4 u_NormalToWorld;
uniform mat4x4 u_NormalToView;

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 texcoord;

struct VS_Output
{
    vec4 color;
};

out vec3 vsPosition;
out vec3 vsNormal;

void main(void)
{
    vsNormal = normalize( u_NormalToWorld * vec4(Normal,0) ).xyz;
    vsPosition  = (u_World * vec4(Position, 1.0)).xyz;

    gl_Position   = u_Projection * u_View * u_World * vec4(Position, 1.0);
}