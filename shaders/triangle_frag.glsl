#version 410 core

// Inputs : vertex position and normal (used for diffuse lighting)
in vec3 normal;
in vec3 pos;

// Shader Parameters
uniform vec3 color1;                // color 1 from the UI
uniform vec3 color2;                // color 2 from the UI
uniform vec3 lightDirection;        // light direction
uniform vec3 lightColor;            // light color
uniform int mode;                   // render mode
uniform float minHeight;            // height of the lowest vertex
uniform float maxHeight;            // height of the highest vertex

// Output : Fragment Color
out vec4 outColor;


//*******************HELPER FUNCTIONS*********************

// RGB to HSV color conversion function.
// Color components are in the [0, 1] range.
vec3 rgbToHSV(vec3 rgbColor) {
    float r = rgbColor.x;
    float g = rgbColor.y;
    float b = rgbColor.z;
    
    // Compute max and min RGB components
    float cMax = max(r, max(g, b));
    float cMin = min(r, min(g, b));
    float delta = cMax - cMin;

    // Compute H component
    float hue = 0.0;
    if (delta > 0.0) {
        if (cMax == r) {
            hue = mod((g - b) / delta, 6.0);
        } else if (cMax == g) {
            hue = (b - r) / delta + 2.0;
        } else {
            hue = (r - g) / delta + 4.0;
        }
        hue /= 6.0; // Normalize hue to range [0, 1]
    }

    // Compute S and V components
    float saturation = (cMax > 0.0) ? delta / cMax : 0.0;
    float value = cMax;

    return vec3(hue, saturation, value);
}

// HSV to RGB color conversion function.
// Color components are in the [0, 1] range.
vec3 hsvToRGB(vec3 hsvColor) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(hsvColor.xxx + K.xyz) * 6.0 - K.www);
    return hsvColor.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), hsvColor.y);
}

// Compute diffuse lighting
vec3 computeDiffuseLighting(vec3 Id, vec3 Kd, vec3 normal, vec3 light) {
    float intensity = max(dot(normalize(normal), normalize(light)), 0.0);
    return Id * Kd * intensity;
}

//********************************************************



void main() {
    // Compute the surface color based on the render mode
    vec3 surfaceColor;
    switch(mode){

        case 1: // Mode 1 : Hue gradient based on height.
            float alpha1 = (pos.y - minHeight)/(maxHeight - minHeight);
            vec3 hsvColor1 = rgbToHSV(color1);
            vec3 hsvColor2 = rgbToHSV(color2);
            surfaceColor = hsvToRGB(vec3(mix(hsvColor1.x, hsvColor2.x, alpha1), hsvColor1.y, hsvColor1.z));
            break;  
            
        default: // Mode 0 : Fixed color
            surfaceColor = color1;
            break;
    }

    // Compute diffuse lighting and output final color.
    vec3 finalColor = computeDiffuseLighting(lightColor, surfaceColor, normal, lightDirection);
    outColor = vec4(finalColor, 1.0);
}
