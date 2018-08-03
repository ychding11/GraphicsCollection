#version 430

layout(triangles) in;
layout(line_strip, max_vertices = 6) out;

uniform mat4x4 u_World;
uniform mat4x4 u_View;
uniform mat4x4 u_Projection;
uniform mat4x4 u_NormalToWorld;
uniform mat4x4 u_NormalToView;

uniform float u_NormalLength;

in Vertex
{
    vec4 normal;
    vec4 color;
} vertexData[];

out vec4 vertex_color;

void main()
{
    int i;
    for (i = 0; i < gl_in.length(); i++)
    {
        vec3 P = gl_in[i].gl_Position.xyz;
        vec3 N = vertexData[i].normal.xyz;

        gl_Position  = u_Projection * u_View * u_World * vec4(P, 1.0);
        vertex_color = vertexData[i].color;
        EmitVertex();

        gl_Position = u_Projection * u_View * u_World  * vec4(P + N * u_NormalLength, 1.0);
        vertex_color = vertexData[i].color;
        EmitVertex();

        EndPrimitive();
    }
}
