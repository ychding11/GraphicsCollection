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

out vec4 vsPosition;
out vec4 vsColor;
out vec4 vsNormal;

void main(void)
{
    vec4 diffuse = vec4(0.9, 0.8, 0.7, 1.0);

    vec3 lightDir = vec3(0, 1, 0);
    vec3 normal   = normalize( u_NormalToWorld * vec4(Normal,0) ).xyz;
    float NdotL   = max(dot(normal, lightDir), 0.0);
    vsColor = NdotL * diffuse;
    vsNormal = vec4(normal, 1.f);
    gl_Position   = u_Projection * u_View * u_World * vec4(Position, 1.0);
    vsPosition = gl_Position;
}