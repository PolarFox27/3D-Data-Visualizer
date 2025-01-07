#version 410 core

// Geometry : 1 vertex --> 1 line (2 vertices on screen)
layout(points) in;
layout(line_strip, max_vertices = 2) out;

// Shader Parameter : MVP matrix.
uniform mat4 mvp;

// Output: vertex position.
out vec3 pos;

void main() {
    vec4 point = gl_in[0].gl_Position;

    // Project the point onto the horizontal plane (y = 0)
    vec4 projectedPoint = vec4(point.x, 0, point.z, point.w);

    // Emit the original point transformed by the MVP matrix
    gl_Position = mvp * point;
    pos = point.xyz;
    EmitVertex();

    // Emit the projected point transformed by the MVP matrix
    gl_Position = mvp * projectedPoint;
    pos = projectedPoint.xyz;
    EmitVertex();

    // End the primitive (line segment)
    EndPrimitive();
}
