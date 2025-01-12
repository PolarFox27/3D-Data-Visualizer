#version 410 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D heightmap;  // Heightmap texture
uniform vec3 aabbMin;
uniform vec3 aabbMax;
uniform int maxSteps;
uniform mat4 mvp;
uniform vec3 color1;
uniform vec3 color2;
uniform float renderHeight;

// Function to compute ray-AABB intersection
bool intersectAABB(vec3 origin, vec3 dir, vec3 boxMin, vec3 boxMax, out float tNear, out float tFar) {
    tNear = -1e20;
    tFar = 1e20;

    for (int i = 0; i < 3; i++) {
        if (abs(dir[i]) < 1e-6) {
            // Ray is parallel to the slab
            if (origin[i] < boxMin[i] || origin[i] > boxMax[i]) {
                return false;
            }
        } else {
            // Compute intersection distances
            float t1 = (boxMin[i] - origin[i]) / dir[i];
            float t2 = (boxMax[i] - origin[i]) / dir[i];

            if (t1 > t2) {
                float temp = t1;
                t1 = t2;
                t2 = temp;
            }

            tNear = max(tNear, t1);
            tFar = min(tFar, t2);

            if (tNear > tFar) return false;
        }
    }
    return true;
}

// Function to sample height from the heightmap texture
float getHeight(vec2 uv) {
    vec4 color = texture(heightmap, uv);
    float grayscale = 0.299f * color.r
                    + 0.587f * color.g
                    + 0.114f * color.b;
    return grayscale * renderHeight;
}

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

// Compute the color based on the height by creating a Hue gradient between the colors.
vec3 computeHeightGradient(float height, float minimum, float maximum, vec3 downColor, vec3 upColor){
    float alpha1 = (height - minimum)/(maximum - minimum);
    vec3 hsvColor1 = rgbToHSV(downColor);
    vec3 hsvColor2 = rgbToHSV(upColor);
    return hsvToRGB(vec3(mix(hsvColor1.x, hsvColor2.x, alpha1), hsvColor1.y, hsvColor1.z));
}

float getTerrainHeightAtPos(vec3 pos){
    // Convert world coordinates to heightmap UV coordinates
    vec2 uv = (pos.xz - aabbMin.xz) / (aabbMax.xz - aabbMin.xz);

    // Sample height from the heightmap
    return getHeight(uv);
}

//********************************************************

// Ray tracing function
void main() {
    // Calculate ray origin and direction in world space
    vec4 ndc = vec4(TexCoord * 2.0 - 1.0, 1.0, 1.0);
    vec4 rayClip = inverse(mvp) * ndc;
    rayClip = rayClip / rayClip.w;

    vec4 rayOrigin = inverse(mvp) * vec4(0.0, 0.0, 0.0, 1.0);  // Camera position in world space
    rayOrigin = rayOrigin / rayOrigin.w;
    vec3 rayDir = normalize(rayClip.xyz - rayOrigin.xyz);
    vec3 cameraPos = rayOrigin.xyz;

    // Intersect ray with the AABB
    float tNear, tFar;
    if (!intersectAABB(cameraPos, rayDir, aabbMin, aabbMax, tNear, tFar)) {
        FragColor = vec4(0.0, 0.0, 0.0, 0.0);  // No intersection with the terrain
        return;
    }

    // Compute step size to traverse the AABB in 100 steps
    float stepSize = (tFar - tNear) / float(maxSteps);

    vec3 firstPos = cameraPos + tNear*rayDir;
    float firstHeight = getTerrainHeightAtPos(firstPos);
    bool isUnder = firstPos.y < firstHeight;
    

    // March along the ray through the AABB
    for (int i = 1; i < maxSteps + 1; i++) {
        float t = tNear + stepSize * float(i);
        vec3 pos = cameraPos + t * rayDir;
        float height = getTerrainHeightAtPos(pos);

        if ((pos.y <= height && !isUnder) || (pos.y >= height && isUnder)) {
            // If the ray hits the terrain surface, output the color
            FragColor = vec4(computeHeightGradient(height, aabbMin.y, aabbMax.y, color1, color2), 1.0);
            return;
        }
    }

    // No hit: output background color
    FragColor = vec4(0.0, 0.0, 0.0, 0.0);
}
