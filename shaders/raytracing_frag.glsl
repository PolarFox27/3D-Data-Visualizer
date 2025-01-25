#version 410 core

// Inputs : texture coordinates
in vec2 TexCoord;

// Shader Parameters
uniform sampler2D heightMap;                // Height map texture
uniform sampler2D normalMap;                // Normal map texture
uniform sampler2D colorMap;                 // Color map texture
uniform sampler2D edgeMap;                  // Edge map texture for valleys and peaks
uniform vec3 aabbMin;                       // corner 1 of the terrain bounding box
uniform vec3 aabbMax;                       // corner 2 of the terrain bounding box
uniform int maxSteps;                       // the amount of steps to perform during ray marching
uniform mat4 mvp;                           // MVP matrix
uniform int lightAmount;                    // amount of lights
uniform int mode;                           // render mode
uniform float renderHeight;                 // the maximum render height for a pixel
uniform bool binarySearch;                  // whether to use binary search

layout(std140) uniform LightData {
    vec4 lightPos[20];                // Array of max 20 Lights. (vec4 is used for memory alignment)
    vec4 lightColor[20];
};

// Output : Fragment Color
out vec4 FragColor;



//*******************RAY TRACING HELPER FUNCTIONS*********************

// Find the intersection point of a ray with the AABB.
bool intersectAABB(vec3 origin, vec3 dir, out float tNear, out float tFar) {
    tNear = -1e20;
    tFar = 1e20;

    // Compute the intersections with the 6 planes defining the bounding box
    for (int i = 0; i < 3; i++) {
        if (abs(dir[i]) < 1e-6) { // Check if ray is parallel to the box
            if (origin[i] < aabbMin[i] || origin[i] > aabbMax[i]) {
                return false;
            }
        } else {
            // Compute intersection distances
            float t1 = (aabbMin[i] - origin[i]) / dir[i];
            float t2 = (aabbMax[i] - origin[i]) / dir[i];

            // Order the intersection distances
            if (t1 > t2) {
                float temp = t1;
                t1 = t2;
                t2 = temp;
            }

            tNear = max(tNear, t1);
            tFar = min(tFar, t2);

            if (tNear > tFar) return false; // Invalid intersection points
        }
    }
    return true;
}

// Compute the terrain height from the height map.
float getTerrainHeightAtPos(vec3 pos) {
    vec2 uv = (pos.xz - aabbMin.xz) / (aabbMax.xz - aabbMin.xz);
    vec4 color = texture(heightMap, uv);
    float grayscale = 0.299f * color.r
                    + 0.587f * color.g
                    + 0.114f * color.b;
    return grayscale * renderHeight;
}

// Compute the terrain normal from the normal map.
vec3 getTerrainNormalAtPos(vec3 pos) {
    vec2 uv = (pos.xz - aabbMin.xz) / (aabbMax.xz - aabbMin.xz);
    return texture(normalMap, uv).xyz;
}

// Compute the edge from the edge map.
vec3 getEdgeAtPos(vec3 pos) {
    vec2 uv = (pos.xz - aabbMin.xz) / (aabbMax.xz - aabbMin.xz);
    return texture(edgeMap, uv).xyz;
}

// Compute the ray attributes for the ray tracing based on the texture coordinates.
void computeRayAttributes(out vec3 origin, out vec3 direction){
    // Find pixel position in the near clipping plane.
    vec4 ndc = vec4(TexCoord * 2.0 - 1.0, 1.0, 1.0);
    vec4 rayClip = inverse(mvp) * ndc;
    rayClip = rayClip / rayClip.w;

    // Find camera position in world space.
    vec4 rayOrigin = inverse(mvp) * vec4(0.0, 0.0, 0.0, 1.0);
    rayOrigin = rayOrigin / rayOrigin.w;
    
    // Output ray attributes
    direction = normalize(rayClip.xyz - rayOrigin.xyz);
    origin = rayOrigin.xyz;
}

//********************************************************************



//*********************LIGHTING HELPER FUNCTIONS**********************

// Compute the color based on a color map
vec3 computeGradient(float value, float minimum, float maximum){
    float alpha = (value - minimum)/(maximum - minimum);
    return texture(colorMap, vec2(alpha, 0)).xyz;
}

// Compute diffuse lighting
vec3 computeDiffuseLighting(vec3 Id, vec3 Kd, vec3 pos, vec3 light) {
    vec3 normal = getTerrainNormalAtPos(pos);
    float intensity = max(dot(normalize(normal), normalize(light)), 0.0);
    return Id * Kd * intensity;
}

vec3 computeColorAtPos(vec3 pos){
    // Compute the surface color based on the render mode
    vec3 surfaceColor;
    switch(mode){

        case 1: // Mode 1 : Hue gradient based on height.
            surfaceColor = computeGradient(getTerrainHeightAtPos(pos), 0, renderHeight);
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
    return finalColor;
}

//********************************************************************


vec3 rayBinarySearch(vec3 pos1, vec3 pos2, bool isUnder){
    for(int i = 0; i < 20; i++){
        vec3 middle = (pos2 + pos1)/2;
        float height = getTerrainHeightAtPos(middle);
        if ((middle.y <= height && !isUnder) || (middle.y >= height && isUnder)){
            pos2 = middle;
        }
        else{
            pos1 = middle;
        }
    }
    return pos1;
}


vec3 rayMarching(vec3 origin, vec3 direction, float tNear, float tFar, out bool hit) {
    // Compute step size
    float stepSize = (tFar - tNear) / float(maxSteps);

    // Check if ray arrives from above or below terrain
    vec3 firstPos = origin + tNear*direction;
    float firstHeight = getTerrainHeightAtPos(firstPos);
    bool isUnder = firstPos.y < firstHeight;
    

    // March along the ray through the AABB
    for (int i = 1; i < maxSteps + 1; i++) {
        float t = tNear + stepSize * float(i);
        vec3 pos = origin + t * direction;
        float height = getTerrainHeightAtPos(pos);

        if ((pos.y <= height && !isUnder) || (pos.y >= height && isUnder)) {
            // If the ray crosses the terrain surface, output the color
            hit = true;
            if(binarySearch){
                return rayBinarySearch(pos - stepSize*direction, pos, isUnder);
            }
            return pos;
        }
    }
    hit = false;
    return vec3(0);
}


void main() {
    // Create ray for ray tracing
    vec3 rayOrigin, rayDirection;
    computeRayAttributes(rayOrigin, rayDirection);

    // Try intersecting ray with the AABB
    float tNear, tFar;
    if (!intersectAABB(rayOrigin, rayDirection, tNear, tFar)) {
        FragColor = vec4(0.0);  // No intersection with the terrain -> no color
        return;
    }

    bool hit;
    vec3 pos = rayMarching(rayOrigin, rayDirection, tNear, tFar, hit);

    if(hit){
        FragColor = vec4(computeColorAtPos(pos), 1.0f);
        return;
    }

    // No hit -> no color
    FragColor = vec4(0.0);
}
