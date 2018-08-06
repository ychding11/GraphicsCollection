#version 430

uniform int u_DiagType;

out vec4  pixelColor;

in vec4 vsPosition;
in vec4 vsColor;
in vec4 vsNormal;

void main()
{
  if (u_DiagType == 0) // non-diag
  {
    pixelColor = vsColor;
  }
  else if (u_DiagType == 1) //position
  {
    pixelColor = vsPosition;
  }
  else if (u_DiagType == 2) //normal
  {
    pixelColor = vsNormal;
  }
  else //fault 
  {
    pixelColor = vec4(1.f, 0.f, 0.f, 1.f);
  }
}
