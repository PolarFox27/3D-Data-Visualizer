#version 410

layout(location = 0) in vec3 aPos; // Vertex attribute for position

void main()
{
    gl_Position = vec4(aPos, 1.0f);
}
