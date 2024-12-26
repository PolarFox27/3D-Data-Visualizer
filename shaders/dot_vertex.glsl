#version 410

layout(location = 0) in vec3 aPos; // Vertex attribute for position
uniform mat4 mvp; // MVP matrix

out vec3 pos; // dot position for the next shader stage


void main()
{
    gl_Position = mvp * vec4(aPos, 1.0f);
    pos = aPos;
}
