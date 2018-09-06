#version 430

out vec4 outColor;

uniform vec3 u_LightIntensity;

void main()
{
  {
	float f = 1.f;
	outColor = vec4(u_LightIntensity, 1.f) * f;
  }
}