#version 410 core
layout(lines_adjacency) in;
layout(triangle_strip, max_vertices = 6) out;

out vec3 fragNormal;

uniform mat4 mvp; // MVP matrix

// Compute the normal of the triangle
vec3 computeNormal(vec3 v0, vec3 v1, vec3 v2) {
    return normalize(cross(v1 - v0, v2 - v0));
}

// Compute the length of the difference between the 2 vectors
float computeDifference(vec3 normal1, vec3 normal2){
    return length(normal1 - normal2);
}

void main() {
    vec3 v0 = gl_in[0].gl_Position.xyz;
    vec3 v1 = gl_in[1].gl_Position.xyz;
    vec3 v2 = gl_in[2].gl_Position.xyz;
    vec3 v3 = gl_in[3].gl_Position.xyz;

    // Compute the normal of the triangles for the 2 possible diagonals
    vec3 normal1 = computeNormal(v0, v1, v2);
    vec3 normal2 = computeNormal(v2, v3, v0);
    vec3 normal3 = computeNormal(v0, v1, v3);
    vec3 normal4 = computeNormal(v2, v3, v1);

    // Choose the diagonal which minimizes the difference between the triangle normals
    if(computeDifference(normal1, normal2) <= computeDifference(normal3, normal4)) {  // Diagonal v0 - v2 is better
        gl_Position = mvp * vec4(v0, 1.0); fragNormal = normal1; EmitVertex();
        gl_Position = mvp * vec4(v1, 1.0); fragNormal = normal1; EmitVertex();
        gl_Position = mvp * vec4(v2, 1.0); fragNormal = normal1; EmitVertex();
        EndPrimitive();

        gl_Position = mvp * vec4(v2, 1.0); fragNormal = normal2; EmitVertex();
        gl_Position = mvp * vec4(v3, 1.0); fragNormal = normal2; EmitVertex();
        gl_Position = mvp * vec4(v0, 1.0); fragNormal = normal2; EmitVertex();
        EndPrimitive();
    }
    else {                                                                           // Diagonal v1 - v3 is better
        gl_Position = mvp * vec4(v0, 1.0); fragNormal = normal3; EmitVertex();
        gl_Position = mvp * vec4(v1, 1.0); fragNormal = normal3; EmitVertex();
        gl_Position = mvp * vec4(v3, 1.0); fragNormal = normal3; EmitVertex();
        EndPrimitive();

        gl_Position = mvp * vec4(v2, 1.0); fragNormal = normal4; EmitVertex();
        gl_Position = mvp * vec4(v3, 1.0); fragNormal = normal4; EmitVertex();
        gl_Position = mvp * vec4(v1, 1.0); fragNormal = normal4; EmitVertex();
        EndPrimitive();
    }
}
