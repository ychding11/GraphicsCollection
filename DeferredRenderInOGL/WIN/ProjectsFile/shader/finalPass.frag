#version 430

/////////////////////////////////////
// Uniforms, Attributes, and Outputs
////////////////////////////////////
uniform mat4 u_World;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform mat4 u_NormalToWorld;
uniform mat4 u_NormalToView;
uniform mat4 u_LightMVP;

uniform sampler2D u_Depthtex;
uniform sampler2D u_Normaltex;
uniform sampler2D u_Positiontex;
uniform sampler2D u_Colortex;
//uniform sampler2D u_shadowmap;

//uniform float u_Far;
//uniform float u_Near;

uniform int u_ScreenWidth;
uniform int u_ScreenHeight;

uniform int u_DiagType;
uniform int u_ShadowOn;

uniform vec3 u_LightPos;
uniform vec3 u_LightColor;
uniform vec3 u_EyePos;


in  vec2 vsTexcoord;
in  vec4 vsPosition;

out vec4 outColor;

/////////////////////////////////////
//				UTILITY FUNCTIONS
/////////////////////////////////////

float zerothresh = 1.0f;
float falloff = 0.1f;
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

// Caculate in world
vec4 myShade( vec3 lightPos, vec3 lightColor, vec4 surfaceColor, vec3 surfacePos, vec3 surfaceNormal, vec3 eyePos )
{
    vec3 L, H, V, R, N;
    L = normalize( lightPos.xyz - surfacePos.xyz );
    N = normalize( surfaceNormal );
    V = normalize( eyePos.xyz - surfacePos.xyz );
    R = 2.0 * dot(L, N) * N - L;
    
	float diffuse  = max(dot(L, N), 0.0f); 
    float specular = pow( max( dot(R, V), 0.0f ), 32); 
	float ambient = 0.1f;
    vec3  color = (ambient + specular + diffuse) * lightColor.xyz * surfaceColor.xyz;
    return vec4 ( color.xyz, 0.0f);
}

void main()
{
    vec4 pos    = samplePos( vsTexcoord );
    vec3 normal = sampleNrm( vsTexcoord );
	vec4 color  = sampleCol( vsTexcoord );


    if (u_DiagType == 0) //Full scene
    {

        vec4 tempColor = myShade(u_LightPos, u_LightColor, color, pos.xyz, normal, u_EyePos);
		if (u_ShadowOn == 1)
		{
			vec4 shadowCoord = u_LightMVP  * pos;
			//if (textureProj(u_shadowmap, shadowCoord.xyw).z < (shadowCoord.z / shadowCoord.w))
			{
				tempColor = color * vec4(0.1f, 0.1f, 0.1f, 1.0f);
			}
		}
		outColor = tempColor;
    }
    else if (u_DiagType == 1) //Position
    {
        outColor = pos;
    }
    else if (u_DiagType == 2) //Normal
    {
        outColor = vec4( normal, 0.0f );
    }
    else if (u_DiagType == 3) //Color
    {
        outColor = color;
    }
    else if (u_DiagType == 4)//shadow
    {
		if (u_ShadowOn == 1)
		{
			vec4 shadowCoord = u_LightMVP  * pos;
			//outColor = vec4( textureProj(u_shadowmap, shadowCoord.xyw) );
		}
        else
			outColor = vec4(1.f, 0.f, 0.f, 1.f);
    }
    else //wrong render red
    {
    }
	outColor = vsPosition;
	outColor = vec4(vsTexcoord, 0, 1);
}