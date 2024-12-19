#version 410 core

uniform vec3 color1;
uniform vec3 color2;
uniform vec3 pos;

out vec4 fragColor;

void main() {
    if(pos.x > pos.z){
        fragColor = vec4(color1, 1.0);
    }
    else {
        fragColor = vec4(color2, 1.0);        
    }
    
}
