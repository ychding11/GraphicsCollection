#version 430

layout (location = 0) in  vec3 Position;
layout (location = 1) in  vec2 Texcoord;

out vec2 vsTexcoord;
out vec4 vsPosition;

void main()
{
    vsTexcoord  = Texcoord;
    vsPosition  = vec4(Position, 1.0f);
}
