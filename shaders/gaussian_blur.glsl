#version 410 core

// Inputs : texture coordinates
in vec2 TexCoord;

// Output : Fragment Color
out vec4 FragColor;

// Shader Parameters
uniform sampler2D inputTexture;     // texture to pass the filter one
uniform vec2 direction;             // 2D direction of the gaussian filter
uniform int size;                   // filter radius (2 * sigma)
const float PI = 3.14159265359;     // PI constant value


// Method to compute the gaussian weight 
float computeGaussianWeight(float x, float sigma) {
    return exp(-(x * x) / (2.0 * sigma * sigma)) / (sqrt(2.0 * PI) * sigma);
}


void main() {
    float sigma = float(size) / 2.0;
    float totalWeight = 0.0;
    vec3 color = vec3(0.0);

    // Compute the weighted sum of colors of the surrounding coords
    for (int i = -size; i <= size; ++i) {
        float weight = computeGaussianWeight(float(i), sigma);
        vec2 offset = direction * float(i);
        color += texture(inputTexture, TexCoord + offset).rgb * weight;
        totalWeight += weight;
    }

    // Returns the weighted sum of the colors
    FragColor = vec4(color / totalWeight, 1.0);
}