#version 430

in  vec2 vsTexcoord;
in  vec4 vsPosition;

out vec4 outColor;

uniform int u_DiagType;

uniform int u_eyePos;

uniform sampler2D u_Depthtex; 
uniform sampler2D u_Normaltex;
uniform sampler2D u_Positiontex;
uniform sampler2D u_Colortex;

void main()
{
	//outColor = vsPosition;
	//outColor = vec4(vsTexcoord, 0, 1);
  if (u_DiagType == 0) // non-diag
  {
	vec3 lightPos1 = vec3(-2.f, 2.f, -2.f);
	vec3 lightPos2 = vec3(-2.f, 2.f, 2.f);
	vec3 lightPos3 = vec3(2.f, 2.f, -2.f);
	vec3 lightPos4 = vec3(2.f, 2.f, 2.f);

	vec3 P = texture(u_Positiontex, vsTexcoord).xyz;
	vec3 N = normalize(texture(u_Normaltex, vsTexcoord).xyz);

	vec3 V = u_eyePos - P;

	vec3 L1 = lightPos1 - P;
	vec3 L2 = lightPos2 - P;
	vec3 L3 = lightPos3 - P;
	vec3 L4 = lightPos4 - P;

	vec3 R1 = 2.f * dot(N, L1) * N - L1;
	vec3 R2 = 2.f * dot(N, L2) * N - L2;
	vec3 R3 = 2.f * dot(N, L3) * N - L3;
	vec3 R4 = 2.f * dot(N, L4) * N - L4;

	float f =pow(max(0.f, dot(R1, V)), 30) + max(0.f, dot(R2, V)) + max(0.f, dot(R3, V)) + max(0.f, dot(R4, V)); 

	outColor = vec4(1.f) * f;
  }
  else if (u_DiagType == 1) //position
  {
	outColor = texture(u_Positiontex, vsTexcoord);
  }
  else if (u_DiagType == 2) //normal
  {
	outColor = texture(u_Normaltex, vsTexcoord);
  }
  else if (u_DiagType == 3) //color
  {
	outColor = texture(u_Colortex, vsTexcoord);
  }
  else if (u_DiagType == 4) //shadow
  {
	outColor = texture(u_Depthtex, vsTexcoord);
  }
  else //fault case 
  {
    outColor  = vec4(1.f, 0.f, 0.f, 1.f);
  }
}