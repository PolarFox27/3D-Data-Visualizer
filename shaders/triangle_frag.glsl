#version 410 core

// Inputs : vertex position
in vec3 pos;

// Shader Parameters
uniform int lightAmount;                    // amount of lights
uniform int mode;                           // render mode
uniform float renderHeight;                 // height of the highest vertex
uniform sampler2D normalMap;                // normal map texture
uniform sampler2D colorMap;                 // color map texture
uniform sampler2D edgeMap;                  // Edge map texture for valleys and peaks
uniform float renderSize;                   // render size to compute normal from position

layout(std140) uniform LightData {
    vec4 lightPos[20];                      // Array of max 20 Lights. (vec4 is used for memory alignment)
    vec4 lightColor[20];
};

// Output : Fragment Color
out vec4 outColor;


//*******************HELPER FUNCTIONS*********************



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

// Compute diffuse lighting
vec3 computeDiffuseLighting(vec3 Id, vec3 Kd, vec3 position, vec3 light) {
    vec3 normal = getTerrainNormalAtPos(position);
    float intensity = max(dot(normalize(normal), normalize(light)), 0.0);
    return Id * Kd * intensity;
}

//********************************************************



void main() {
    // Compute the surface color based on the render mode
    vec3 surfaceColor;
    switch(mode){

        case 1: // Mode 1 : Hue gradient based on height.
            surfaceColor = computeGradient(pos.y, 0, renderHeight);
            break;  

        case 2: // Mode 2 : Hue gradient based on slope steepness
            vec3 normal = getTerrainNormalAtPos(pos);
            float value = 1.0 - abs(dot(normal, vec3(0, 1, 0)));
            surfaceColor = computeGradient(value, 0.0, 1.0);
            break;

        case 3: // Mode 3 : Different colors for peaks and valleys
            surfaceColor = computeGradient(getEdgeAtPos(pos).y, 1, 0);
            break;
            
        default: // Mode 0 : Fixed color
            surfaceColor = computeGradient(0.5, 0, 1);
            break;
    }

    // Compute diffuse lighting for all lights and output final color.
    vec3 finalColor = vec3(0);
    for (int i = 0; i < lightAmount; i++){
        finalColor += computeDiffuseLighting(lightColor[i].xyz, surfaceColor, pos, lightPos[i].xyz);
    }
    outColor = vec4(finalColor, 1.0);
}
