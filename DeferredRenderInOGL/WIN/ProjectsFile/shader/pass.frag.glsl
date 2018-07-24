#version 430

uniform float u_Far;
uniform float u_Near;

uniform vec3 u_Color;
uniform float u_shininess;

uniform sampler2D u_colorTex;
uniform sampler2D u_bumpTex;
uniform int u_bTextured;
uniform int u_bBump;

uniform mat4x4 u_NormalToWorld;
uniform mat4x4 u_NormalToView;

in vec3 fs_Normal;
in vec4 fs_Position;
in vec2 fs_texcoord;

layout (location = 0) out vec4 out_Normal;
layout (location = 1) out vec4 out_Position;
layout (location = 2) out vec4 out_Color;

void main(void)
{
    out_Position = fs_Position;
    out_Normal = vec4( normalize(fs_Normal), 0.0f);
	if( u_bBump == 1 )
	    out_Normal = u_NormalToWorld * vec4( texture( u_bumpTex, fs_texcoord ).rgb, 0.0f );
    out_Color = vec4( u_Color, u_shininess );
	if( u_bTextured == 1 )
	    out_Color.rgb = texture( u_colorTex, fs_texcoord ).rgb;
}
