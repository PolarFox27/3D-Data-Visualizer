#version 410 core

// Geometry : 4 vertices --> 2 triangles (6 vertices on screen)
layout(lines_adjacency) in;
layout(triangle_strip, max_vertices = 6) out;

// Shader Parameter : MVP matrix.
uniform mat4 mvp;

// Output: vertex position and normal.
out vec3 pos;



//*******************HELPER FUNCTIONS*********************

// Compare lines and check if the first line is above the second line
bool compareLines(vec3 p1, vec3 p2, vec3 q1, vec3 q2) {
    float pHeight = (p1.y + p2.y) * 0.5;
    float qHeight = (q1.y + q2.y) * 0.5;
    return pHeight > qHeight;
}

//********************************************************



void main() {
// Retrieve the positions of the 4 vertices
    vec3 v0 = gl_in[0].gl_Position.xyz;
    vec3 v1 = gl_in[1].gl_Position.xyz;
    vec3 v2 = gl_in[2].gl_Position.xyz;
    vec3 v3 = gl_in[3].gl_Position.xyz;    

    // Choose the heighest diagonal
    if(compareLines(v0, v2, v1, v3)) {  // Diagonal v0 - v2 is better
        
        // Create triangle 1 on screen
        gl_Position = mvp * vec4(v0, 1.0); pos = v0; EmitVertex();
        gl_Position = mvp * vec4(v1, 1.0); pos = v1; EmitVertex();
        gl_Position = mvp * vec4(v2, 1.0); pos = v2; EmitVertex();
        EndPrimitive();

        // Create triangle 2 on screen
        gl_Position = mvp * vec4(v2, 1.0); pos = v2; EmitVertex();
        gl_Position = mvp * vec4(v3, 1.0); pos = v3; EmitVertex();
        gl_Position = mvp * vec4(v0, 1.0); pos = v0; EmitVertex();
        EndPrimitive();
    }
    else {                                                                           // Diagonal v1 - v3 is better

        // Create triangle 1 on screen
        gl_Position = mvp * vec4(v0, 1.0); pos = v0; EmitVertex();
        gl_Position = mvp * vec4(v1, 1.0); pos = v1; EmitVertex();
        gl_Position = mvp * vec4(v3, 1.0); pos = v3; EmitVertex();
        EndPrimitive();

        // Create triangle 2 on screen
        gl_Position = mvp * vec4(v2, 1.0); pos = v2; EmitVertex();
        gl_Position = mvp * vec4(v3, 1.0); pos = v3; EmitVertex();
        gl_Position = mvp * vec4(v1, 1.0); pos = v1; EmitVertex();
        EndPrimitive();
    }
}
