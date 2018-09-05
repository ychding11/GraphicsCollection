#version 430

uniform int  u_DiagType;
uniform vec3 u_EyePos;

out vec4  pixelColor;

in vec3 vsPosition;
in vec3 vsNormal;

void main()
{
  if (u_DiagType == 0) // non-diag
  {
   	vec3 lightPos1 = vec3(-2.f, 2.f, -2.f);
	vec3 lightPos2 = vec3(-2.f, 2.f, 2.f);
	vec3 lightPos3 = vec3(2.f, 2.f, -2.f);
	vec3 lightPos4 = vec3(2.f, 2.f, 2.f);

	vec3 P = vsPosition; 
	vec3 N = vsNormal; 

	vec3 V = u_EyePos - P;

	vec3 L1 = lightPos1 - P;
	vec3 L2 = lightPos2 - P;
	vec3 L3 = lightPos3 - P;
	vec3 L4 = lightPos4 - P;

	vec3 R1 = 2.f * dot(N, L1) * N - L1;
	vec3 R2 = 2.f * dot(N, L2) * N - L2;
	vec3 R3 = 2.f * dot(N, L3) * N - L3;
	vec3 R4 = 2.f * dot(N, L4) * N - L4;

	float f =pow(max(0.f, dot(R1, V)), 30) + max(0.f, dot(R2, V)) + max(0.f, dot(R3, V)) + max(0.f, dot(R4, V)); 

	pixelColor = vec4(1.f) * f;
 
  }
  else if (u_DiagType == 1) //position
  {
    pixelColor = vec4(vsPosition, 1.f);
  }
  else if (u_DiagType == 2) //normal
  {
    pixelColor = vec4(vsNormal, 1.f);
  }
  else //fault 
  {
    pixelColor = vec4(1.f, 0.f, 0.f, 1.f);
  }
}
