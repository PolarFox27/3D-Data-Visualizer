#version 410 core

// Inputs : position and texture coordinates from vertex attributes in VBO
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

// Output : texture coordinates for the next shader stage
out vec2 TexCoord;

void main()
{
    // Set vertex position on screen, and pass texture coordinates to next stage.
    gl_Position = vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
