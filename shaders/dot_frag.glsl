#version 410 core

// Input: Fragment Position
in vec3 pos;                        // dot position


// Shader Parameters
uniform int mode;                   // rendering mode
uniform vec3 cameraPos;             // camera position

uniform float renderHeight;         // render height
uniform float minDistanceToCamera;  // distance to the closest dot
uniform float maxDistanceToCamera;  // distance to the farest dot
uniform sampler2D colorMap;         // color map texture


// Output: Fragment Color
out vec4 outColor;



//*******************HELPER FUNCTIONS*********************

const vec3 BLACK = vec3(0, 0, 0);

// Compute the color based on a color map
vec3 computeGradient(float value, float minimum, float maximum){
    float alpha = (value - minimum)/(maximum - minimum);
    return texture(colorMap, vec2(alpha, 0)).xyz;
}

//********************************************************



void main() {
    // Fragment Color will depend on render mode
    switch(mode){

        case 1: // Mode 1 : gradient based on height and darkening based on distance to camera.
            
            // Intermediate color computed with the height gradient
            vec3 intermediateColor = computeGradient(pos.y, 0, renderHeight);

            // Compute darkening gradient alpha parameter based on the fragment distance to camera.
            float distanceToCamera = clamp(distance(cameraPos, pos), minDistanceToCamera, maxDistanceToCamera);
            float alpha2 = (distanceToCamera - minDistanceToCamera)/(maxDistanceToCamera - minDistanceToCamera);
            
            // Compute and output final color
            vec3 finalColor = mix(intermediateColor, BLACK, alpha2);
            outColor = vec4(finalColor, 1);
            return;            
        
        
        default: // Mode 0 : Fixed color
            outColor = vec4(computeGradient(0.5, 0, 1), 1);
            return;
    }
}
