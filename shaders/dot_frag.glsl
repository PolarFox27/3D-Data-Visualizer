#version 410 core

in vec3 pos;               // dot position
uniform vec3 cameraPos;    // camera position

// UI Parameters
uniform vec3 color1;       // dot color 1
uniform vec3 color2;       // dot color 2
uniform int mode;          // dot rendering mode

uniform float minHeight;   // height of the lowest dot
uniform float maxHeight;   // height of the highest dot
uniform float minDistanceToCamera; // distance to the closest dot
uniform float maxDistanceToCamera; // distance to the farest dot

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


void main() {
    switch(mode){
        case 1: // Mode 1 : Color gradient based on height and on distance to camera.
            float alpha1 = (pos.y - minHeight)/(maxHeight - minHeight);
            vec3 hsvColor1 = rgbToHSV(color1);
            vec3 hsvColor2 = rgbToHSV(color2);

            vec3 color = hsvToRGB(vec3(mix(hsvColor1.x, hsvColor2.x, alpha1), hsvColor1.y, hsvColor1.z));
            float distanceToCamera = clamp(distance(cameraPos, pos), minDistanceToCamera, maxDistanceToCamera);
            float alpha2 = (distanceToCamera - minDistanceToCamera)/(maxDistanceToCamera - minDistanceToCamera);

            outColor = vec4(mix(color, vec3(0, 0, 0), alpha2), 1);
            return;            
        default: // Mode 0 : Fixed color
            outColor = vec4(color1, 1);
            return;
    }
}
