#version 430

out vec4     pixelColor;

in vec4 vsColor;
in vec4 vsNormal;

void main()
{
  pixelColor = vsNormal;
}
