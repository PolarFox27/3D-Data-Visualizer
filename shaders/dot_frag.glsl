#version 410 core

// Input: Fragment Position
in vec3 pos;                        // dot position


// Shader Parameters
uniform int mode;                   // rendering mode
uniform vec3 cameraPos;             // camera position

uniform float renderHeight;         // render height
uniform float renderSize;           // render size to compute normal from position
uniform float minDistanceToCamera;  // distance to the closest dot
uniform float maxDistanceToCamera;  // distance to the farest dot
uniform sampler2D normalMap;        // normal map texture
uniform sampler2D colorMap;         // color map texture
uniform sampler2D edgeMap;          // Edge map texture for valleys and peaks


// Output: Fragment Color
out vec4 outColor;



//*******************HELPER FUNCTIONS*********************

const vec3 BLACK = vec3(0, 0, 0);

// Compute the color based on a color map
vec3 computeGradient(float value, float minimum, float maximum){
    float alpha = (value - minimum)/(maximum - minimum);
    return texture(colorMap, vec2(alpha, 0)).xyz;
}

// Compute the terrain normal from the normal map.
vec3 getTerrainNormalAtPos(vec3 pos) {
    vec2 uv = vec2(pos.x/renderSize + 0.5, pos.z/renderSize + 0.5);
    return texture(normalMap, uv).xyz;
}

// Compute the edge from the edge map.
vec3 getEdgeAtPos(vec3 pos) {
    vec2 uv = vec2(pos.x/renderSize + 0.5, pos.z/renderSize + 0.5);
    return texture(edgeMap, uv).xyz;
}

vec3 computeColorAtPos(vec3 pos){    
    // Compute darkening gradient alpha parameter based on the fragment distance to camera.
    float distanceToCamera = clamp(distance(cameraPos, pos), minDistanceToCamera, maxDistanceToCamera);
    float alpha = (distanceToCamera - minDistanceToCamera)/(maxDistanceToCamera - minDistanceToCamera);
    vec3 surfaceColor;
    vec3 intermediateColor;

    switch(mode){

        case 1: // Mode 1 : gradient based on height.
            intermediateColor = computeGradient(pos.y, 0, renderHeight);
            
            // Compute final color
            surfaceColor = mix(intermediateColor, BLACK, alpha);
            break;  

        case 2: // Mode 2 : gradient based on slope steepness
            vec3 normal = getTerrainNormalAtPos(pos);
            float value = 1.0 - abs(dot(normal, vec3(0, 1, 0)));
            intermediateColor = computeGradient(value, 0.0, 1.0);

            // Compute final color
            surfaceColor = mix(intermediateColor, BLACK, alpha);
            break;

        case 3: // Mode 3 : Different colors for peaks and valleys
            intermediateColor = computeGradient(getEdgeAtPos(pos).y, 0.1, 0);

            // Compute final color
            surfaceColor = mix(intermediateColor, BLACK, alpha);
            break;
            
        default: // Mode 0 : Fixed color
            surfaceColor = computeGradient(0.5, 0, 1);
            break;
    }
    return surfaceColor;
}

//********************************************************



void main() {
    vec3 finalColor = computeColorAtPos(pos);
    outColor = vec4(finalColor, 1.0);
}
