#version 410 core

// Input : texture coordinate
in vec2 TexCoord;

// Shader Parameter : Texture
uniform sampler2D quadTexture;

// Output: Fragment Color
out vec4 outColor;


void main() {
    // Output fragment color based on the texture and the coordinates.
    outColor = texture(quadTexture, TexCoord);
}
