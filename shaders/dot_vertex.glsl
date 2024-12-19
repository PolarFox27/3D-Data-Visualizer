#version 410 core

uniform mat4 mvp;
uniform vec3 position;

void main() {
    gl_Position = mvp * vec4(position, 1.0);
}
