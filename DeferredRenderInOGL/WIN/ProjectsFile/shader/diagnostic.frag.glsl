#version 430

////////////////////////////
//       ENUMERATIONS
////////////////////////////

#define	DISPLAY_DEPTH 0
#define	DISPLAY_NORMAL 1
#define	DISPLAY_POSITION 2
#define	DISPLAY_COLOR 3
#define	DISPLAY_TOTAL 4
#define	DISPLAY_LIGHTS 5

/////////////////////////////////////
// Uniforms, Attributes, and Outputs
////////////////////////////////////
uniform mat4 u_persp;
uniform mat4 u_modelview;
uniform mat4 u_lightMVP;

uniform sampler2D u_Depthtex;
uniform sampler2D u_Normaltex;
uniform sampler2D u_Positiontex;
uniform sampler2D u_Colortex;
uniform sampler2D u_shadowmap;

uniform float u_Far;
uniform float u_Near;
uniform int u_DisplayType;

uniform int u_ScreenWidth;
uniform int u_ScreenHeight;
uniform int u_RenderMode;

uniform vec4 u_Light;
uniform vec3 u_LightColor;
uniform vec3 u_eyePos;

uniform float zerothresh = 1.0f;
uniform float falloff = 0.1f;

in vec2 fs_Texcoord;

out vec4 out_Color;
///////////////////////////////////////


/////////////////////////////////////
//				UTILITY FUNCTIONS
/////////////////////////////////////

//Depth used in the Z buffer is not linearly related to distance from camera
//This restores linear depth
float linearizeDepth(float exp_depth, float near, float far)
{
    return	(2 * near) / (far + near -  exp_depth * (far - near)); 
}

vec3 sampleNrm(vec2 texcoord)
{
    return texture( u_Normaltex, texcoord).xyz;
}

vec4 samplePos(vec2 texcoord)
{
    vec4 pos = texture( u_Positiontex, texcoord );
    return pos;
}

vec4 sampleCol(vec2 texcoord)
{
    return texture(u_Colortex, texcoord);
}

vec3 shade( vec4 light, vec3 lightColor, vec4 vertexColor, vec3 pos, vec3 N )
{
    vec3 L, H, V, R;
	vec3 color;
	float LdotN, blinnTerm;
	float decay = 1;

	N = normalize(N);
	L = light.w > 0 ? light.xyz - pos : light.xyz;
	L = normalize(L);
	V = normalize( -pos );
	H = normalize( L+V );
	
	LdotN = max( dot(L,N), 0 );
	blinnTerm = max( dot(N, H), 0 );
	blinnTerm = pow( blinnTerm, vertexColor.a );

	if( light.w > 0 )
	    decay = 1.0 / distance( light.xyz, pos ) / distance( light.xyz, pos );
	color = decay * ( vertexColor.rgb * LdotN + vec3(1,1,1)*blinnTerm );

	return color;
}

void main()
{
    float exp_depth = texture(u_Depthtex, fs_Texcoord).r;
    float lin_depth = linearizeDepth(exp_depth, u_Near, u_Far);
	
    vec4 pos    = samplePos( fs_Texcoord );
    vec3 normal = sampleNrm( fs_Texcoord );
	vec4 color  = sampleCol( fs_Texcoord );
    if (u_RenderMode == 2)
    {
        out_Color = pos;
        return;
    }
    else if (u_RenderMode == 3)
    {
        out_Color = vec4( normal, 0.0f );
        return;
    }
    else if (u_RenderMode == 4)
    {
        out_Color = vec4( 0.0f, 0.0f, 0.0f, 0.0f );
        return;
    }
    else if (u_RenderMode == 5)
    {
        out_Color = vec4( texture( u_shadowmap, fs_Texcoord ).r, 0.0f, 0.0f, 0.0f );
        return;
    }
    else
    {
    }

	float visibility = 1.0;
	vec4 posEyeSpace = u_modelview * pos;
	vec4 shadowCoord = u_lightMVP  * pos;

	float depthBias =0;
	if( textureProj( u_shadowmap, shadowCoord.xyw ).z < ( shadowCoord.z - depthBias ) / shadowCoord.w  )
	    visibility = 0.2;

    out_Color = visibility * vec4( shade( u_Light, u_LightColor, color, posEyeSpace.xyz/posEyeSpace.w, normal ), 0.0f ); 

	//float s_depth = texture( u_shadowmap, fs_Texcoord ).r;
	//out_Color = vec4( s_depth, s_depth, s_depth, 0 );
}

