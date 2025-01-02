#version 410

// Input : position from vertex attributes in VBO
layout(location = 0) in vec3 aPos;


void main()
{
    // Set OpenGL position in projective space at the vertex position.
    gl_Position = vec4(aPos, 1.0f);
}
