#version 430

in  vec4 vertex_color;
out vec4 outColor;

void main()
{
    outColor = vertex_color;
    //outColor = vec4(1.0f, 0., 0., 1.f);
}
