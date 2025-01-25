#version 410 core

// Inputs : texture coordinates
in vec2 TexCoord;

// Output : Fragment Color
out vec4 FragColor;

// Shader Parameters
uniform sampler2D originalBlurredTexture;
uniform sampler2D largeBlurredTexture;


// Substract the large gaussian blur to the original blur
void main() {
    vec3 original = texture(originalBlurredTexture, TexCoord).rgb;
    vec3 large = texture(largeBlurredTexture, TexCoord).rgb;
    vec3 edge = vec3(original.x, abs(original.y - large.y)*10, original.z);
    FragColor = vec4(edge, 1.0);
}