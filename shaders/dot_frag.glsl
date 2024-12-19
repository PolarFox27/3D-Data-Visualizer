#version 410 core

uniform vec3 color1;
uniform vec3 color2;
uniform vec4 pos;
uniform vec3 worldPos;
uniform int mode;
uniform float minHeight;
uniform float maxHeight;

out vec4 outColor;

void main() {
    switch(mode){
        case 1: // Mode 1 : Color gradient based on height.
            float alpha = (worldPos.y - minHeight)/(maxHeight - minHeight);
            outColor = vec4(mix(color1, color2, alpha), 1);
            return;
        default: // Mode 0 : Fixed color
            outColor = vec4(color1, 1);
            return;
    }
}
