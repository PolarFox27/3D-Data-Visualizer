#version 410 core

// Input: Fragment Position
in vec3 pos;                        // dot position


// Shader Parameters
uniform vec3 color1;                // dot color 1
uniform vec3 color2;                // dot color 2
uniform int mode;                   // rendering mode
uniform vec3 cameraPos;             // camera position

uniform float minHeight;            // height of the lowest dot
uniform float maxHeight;            // height of the highest dot
uniform float minDistanceToCamera;  // distance to the closest dot
uniform float maxDistanceToCamera;  // distance to the farest dot


// Output: Fragment Color
out vec4 outColor;



//*******************HELPER FUNCTIONS*********************

const vec3 BLACK = vec3(0, 0, 0);

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

//********************************************************



void main() {
    // Fragment Color will depend on render mode
    switch(mode){

        case 1: // Mode 1 : Hue gradient based on height and darkening based on distance to camera.
            // Compute hue gradient alpha parameter based on the fragment height.
            float alpha1 = (pos.y - minHeight)/(maxHeight - minHeight);

            // Convert color 1 and 2 to HSV for the hue gradient
            vec3 hsvColor1 = rgbToHSV(color1);
            vec3 hsvColor2 = rgbToHSV(color2);

            // Get RGB color from hue gradient, keeping S and V components constant.
            vec3 intermediateColor = hsvToRGB(vec3(mix(hsvColor1.x, hsvColor2.x, alpha1), hsvColor1.y, hsvColor1.z));

            // Compute darkening gradient alpha parameter based on the fragment distance to camera.
            float distanceToCamera = clamp(distance(cameraPos, pos), minDistanceToCamera, maxDistanceToCamera);
            float alpha2 = (distanceToCamera - minDistanceToCamera)/(maxDistanceToCamera - minDistanceToCamera);
            
            // Compute and output final color
            vec3 finalColor = mix(intermediateColor, BLACK, alpha2);
            outColor = vec4(finalColor, 1);
            return;            
        
        
        default: // Mode 0 : Fixed color
            outColor = vec4(color1, 1);
            return;
    }
}
