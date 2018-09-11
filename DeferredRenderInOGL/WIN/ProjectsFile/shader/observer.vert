#version 430

layout (location = 0) in  vec4 Position;

uniform mat4x4 u_World;
uniform mat4x4 u_View;
uniform mat4x4 u_Projection;

void main()
{
    gl_Position   = u_Projection * u_View * u_World * Position;
    //gl_Position   = Position;
}
