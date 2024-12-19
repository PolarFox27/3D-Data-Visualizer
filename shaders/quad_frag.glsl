#version 330 core
in vec2 vTexCoord;
out vec4 fragColor;

uniform sampler2D imageTexture;

void main() {
    fragColor = texture(imageTexture, vTexCoord);
}
