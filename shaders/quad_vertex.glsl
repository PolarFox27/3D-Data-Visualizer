#version 410 core

// Inputs : position and texture coordinates from vertex attributes in VBO
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

// Shader Parameter : MVP matrix and height
uniform mat4 mvp;
uniform float height;

// Output : texture coordinate for the next shader stage
out vec2 TexCoord;


void main() {
    // Compute vertex position on screen, and pass texture coordinate to next stage.
    gl_Position = mvp * vec4(aPos + vec3(0, height, 0), 1.0);
    TexCoord = aTexCoord;
}
