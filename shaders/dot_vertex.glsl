#version 410

// Input : position from vertex attributes in VBO
layout(location = 0) in vec3 aPos;

// Shader Parameter : MVP matrix
uniform mat4 mvp;

// Output : vertex position for the next shader stage
out vec3 pos;


void main()
{
    // Compute vertex position on screen, and pass position to next stage.
    gl_Position = mvp * vec4(aPos, 1.0f);
    pos = aPos;
}
