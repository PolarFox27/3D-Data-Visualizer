#version 410 core
in vec3 normal;
in vec3 pos;
out vec4 FragColor;


uniform vec3 color1;       // triangle color 1
uniform vec3 color2;       // triangle color 2
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform int mode;
uniform float minHeight;   // height of the lowest dot
uniform float maxHeight;   // height of the highest dot

out vec4 outColor;

vec3 rgbToHSV(vec3 c) {
    float cMax = max(c.x, max(c.y, c.z));
    float cMin = min(c.x, min(c.y, c.z));
    float delta = cMax - cMin;

    float hue = 0.0;
    if (delta > 0.0) {
        if (cMax == c.x) {
            hue = mod((c.y - c.z) / delta, 6.0);
        } else if (cMax == c.y) {
            hue = (c.z - c.x) / delta + 2.0;
        } else {
            hue = (c.x - c.y) / delta + 4.0;
        }
        hue /= 6.0; // Normalize hue to range [0, 1]
    }

    float saturation = (cMax > 0.0) ? delta / cMax : 0.0;
    float value = cMax;

    return vec3(hue, saturation, value);
}

vec3 hsvToRGB(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// Compute diffuse lighting
vec3 computeDiffuseLighting(vec3 Id, vec3 Kd, vec3 normal, vec3 light) {
    float intensity = max(dot(normalize(normal), normalize(light)), 0.0);
    return Id * Kd * intensity;
}

void main() {
    vec3 surfaceColor;
    switch(mode){
        case 1: // Mode 1 : Color gradient based on height.
            float alpha1 = (pos.y - minHeight)/(maxHeight - minHeight);
            vec3 hsvColor1 = rgbToHSV(color1);
            vec3 hsvColor2 = rgbToHSV(color2);
            surfaceColor = hsvToRGB(vec3(mix(hsvColor1.x, hsvColor2.x, alpha1), hsvColor1.y, hsvColor1.z));
            break;            
        default: // Mode 0 : Fixed color
            surfaceColor = color1;
            break;
    }
    FragColor = vec4(computeDiffuseLighting(lightColor, surfaceColor, normal, lightDirection), 1.0);
}
