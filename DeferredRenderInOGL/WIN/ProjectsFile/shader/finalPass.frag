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
	vec4 pos    = texture(u_Positiontex, vsTexcoord);
	vec4 normal = texture(u_Normaltex, vsTexcoord);
	outColor = vec4(10.f) * dot(normal.xyz, u_eyePos - pos.xyz);
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