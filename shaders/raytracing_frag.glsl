#version 410 core

// Inputs : texture coordinates
in vec2 TexCoord;

// Shader Parameters
uniform sampler2D heightMap;        // Height map texture
uniform sampler2D normalMap;        // Normal map texture
uniform vec3 aabbMin;               // corner 1 of the terrain bounding box
uniform vec3 aabbMax;               // corner 2 of the terrain bounding box
uniform int maxSteps;               // the amount of steps to perform during ray marching
uniform mat4 mvp;                   // MVP matrix
uniform vec3 color1;                // color 1 from the UI
uniform vec3 color2;                // color 2 from the UI
uniform int lightAmount;            // amount of lights
uniform int mode;                   // render mode
uniform float renderHeight;         // the maximum render height for a pixel

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

// Compute the color based on the height by creating a Hue gradient between color1 and color2.
vec3 computeHeightGradient(float height){
    float alpha1 = (height - aabbMin.y)/(aabbMax.y - aabbMin.y);
    vec3 hsvColor1 = rgbToHSV(color1);
    vec3 hsvColor2 = rgbToHSV(color2);
    float hue = mix(hsvColor1.x, hsvColor2.x, alpha1);
    vec3 hsvResult = vec3(hue, hsvColor1.y, hsvColor1.z);
    return hsvToRGB(hsvResult);
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
            surfaceColor = computeHeightGradient(getTerrainHeightAtPos(pos));
            break;  
            
        default: // Mode 0 : Fixed color
            surfaceColor = color1;
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

    // Compute step size
    float stepSize = (tFar - tNear) / float(maxSteps);

    // Check if ray arrives from above or below terrain
    vec3 firstPos = rayOrigin + tNear*rayDirection;
    float firstHeight = getTerrainHeightAtPos(firstPos);
    bool isUnder = firstPos.y < firstHeight;
    

    // March along the ray through the AABB
    for (int i = 1; i < maxSteps + 1; i++) {
        float t = tNear + stepSize * float(i);
        vec3 pos = rayOrigin + t * rayDirection;
        float height = getTerrainHeightAtPos(pos);

        if ((pos.y <= height && !isUnder) || (pos.y >= height && isUnder)) {
            // If the ray crosses the terrain surface, output the color
            FragColor = vec4(computeColorAtPos(pos), 1.0);
            return;
        }
    }

    // No hit -> no color
    FragColor = vec4(0.0);
}
