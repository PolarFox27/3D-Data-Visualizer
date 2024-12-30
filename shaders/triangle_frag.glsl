#version 410 core
in vec3 fragNormal;
out vec4 FragColor;

uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform vec3 surfaceColor;

// Compute diffuse lighting
vec3 computeDiffuseLighting(vec3 Id, vec3 Kd, vec3 normal, vec3 light) {
    float intensity = abs(dot(normalize(normal), normalize(light)));
    return Id * Kd * intensity;
}

void main() {
    FragColor = vec4(computeDiffuseLighting(lightColor, surfaceColor, fragNormal, lightDirection), 1.0);
}
