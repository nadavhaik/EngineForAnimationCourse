#version 330

in vec2 texCoord0;
in vec3 normal0;
in vec3 color0;
in vec3 position0;

uniform vec4 lightColor;
uniform sampler2D sampler1;
uniform vec4 lightDirection;
uniform vec3 cameraPos;
uniform int fog_enabled;

out vec4 Color;

void main()
{
    // color of yellow
    Color = vec4(1, 0, 1,0.7);

    if(fog_enabled == 1) {
        float fog_maxdist = 40;
        float fog_mindist = 0.1;
        vec4  fog_colour = vec4(0.7, 0.7, 0.7, 1.0);
        vec3 diff = cameraPos - position0;
        float dist = sqrt(diff.x * diff.x + diff.y + diff.y + diff.z * diff.z);
        float fog_factor = (fog_maxdist - dist) / (fog_maxdist - fog_mindist);
        fog_factor = clamp(fog_factor, 0.0, 1.0);

        Color = mix(fog_colour, Color, fog_factor);
    }
}
