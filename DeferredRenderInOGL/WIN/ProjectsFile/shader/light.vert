#version 430

layout (location = 0) in  vec3 Position;
layout (location = 1) in  vec2 Texcoord;

uniform mat4x4 u_World;
uniform mat4x4 u_View;
uniform mat4x4 u_Projection;

void main()
{
    gl_Position   = u_Projection * u_View * u_World * vec4(Position, 1.0);
}
