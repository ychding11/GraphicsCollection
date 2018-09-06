#version 430

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 texcoord;

out vec4 vsPosition;
out vec4 vsColor;
out vec4 vsNormal;

uniform mat4x4 u_World;
uniform mat4x4 u_View;
uniform mat4x4 u_Projection;
uniform mat4x4 u_NormalToWorld;
uniform mat4x4 u_NormalToView;

uniform vec3 u_LightIntensity;
uniform vec3 u_LightPos;
uniform vec3 u_EyePos;

void main(void)
{
    vec3 diffuse = vec3(0.9, 0.4, 0.5);
	vec3 P = (u_World * vec4(Position, 1.0)).xyz;

    vec3 L = u_LightPos - P; 
	vec3 V = u_EyePos - P;
    vec3 N   = normalize( u_NormalToWorld * vec4(Normal,0.f) ).xyz;

    float NdotL   = max(dot(N, L), 0.f);
	vec3  res = NdotL * diffuse * u_LightIntensity; 

    vsColor = vec4(res, 1.f);

    gl_Position   = u_Projection * u_View * u_World * vec4(Position, 1.0);
    vsNormal = vec4(N, 0.f);
    vsPosition = vec4(P, 1.f);
}