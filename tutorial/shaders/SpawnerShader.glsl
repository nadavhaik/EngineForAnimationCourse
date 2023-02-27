#version 330

in vec2 texCoord0;
in vec3 normal0;
in vec3 color0;
in vec3 position0;

uniform vec4 lightColor;
uniform sampler2D sampler1;
uniform vec4 lightDirection;

out vec4 Color;

void main()
{
    // color of yellow
    Color = vec4(0.98828125, 0.8515625, 0.05078125,0.7);
}
