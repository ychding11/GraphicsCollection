#version 430

uniform float u_Far;
uniform float u_Near;

uniform vec3  u_Color;
uniform float u_Shininess;

uniform sampler2D u_ColorTex;
uniform sampler2D u_BumpTex;

uniform int u_bTextured;
uniform int u_bBump;

uniform mat4x4 u_NormalToWorld;
uniform mat4x4 u_NormalToView;

in vec3 vsNormal;
in vec4 vsPosition;
in vec2 vsTexcoord;
in vec4 vsProjectedPos;


layout (location = 0) out vec4 outNormal;
layout (location = 1) out vec4 outPosition;
layout (location = 2) out vec4 outColor;

void main(void)
{
    outNormal   = vec4( normalize(vsNormal), 0.0f);
    outPosition = vsPosition;
    outColor    = vec4( u_Color, u_Shininess );
	if( u_bBump == 1 )
	    outNormal = u_NormalToWorld * vec4( texture(u_BumpTex, vsTexcoord).rgb, 0.0f );
	if( u_bTextured == 1 )
	    outColor.rgb = texture( u_ColorTex, vsTexcoord ).rgb;
}
