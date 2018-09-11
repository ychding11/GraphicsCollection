#version 430

out vec4 outColor;

void main()
{
  {
	float f = 1.f;
	outColor = vec4(0.f, 1.f, 0.f, 1.f) * f;
  }
}