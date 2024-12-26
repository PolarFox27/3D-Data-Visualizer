#version 410 core

in vec3 pos; // dot position

// UI Parameters
uniform vec3 color1; // dot color 1
uniform vec3 color2; // dot color 2
uniform int mode; // dot rendering mode

uniform float minHeight; // height of the lowest dot
uniform float maxHeight; // height of the highest dot

out vec4 outColor;

void main() {
    switch(mode){
        case 1: // Mode 1 : Color gradient based on height.
            float alpha = (pos.y - minHeight)/(maxHeight - minHeight);
            outColor = vec4(mix(color1, color2, alpha), 1);
            return;
        default: // Mode 0 : Fixed color
            outColor = vec4(color1, 1);
            return;
    }
}
