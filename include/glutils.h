#include <utils.h>

//=============================OpenGL Constants==============================

// Indices needed to render a rectangle as 2 triangles
const unsigned int QUAD_INDICES[] = {
        0, 1, 2,  // First Triangle
        2, 3, 0   // Second Triangle
};

// On-screen positions of the raytracing image plane
const std::vector<float> RAYTRACING_VERTICES = {
    // Positions       // Texture Coords
    -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,  // Bottom-left
     1.0f, -1.0f, 0.0f,  1.0f, 0.0f,  // Bottom-right
     1.0f,  1.0f, 0.0f,  1.0f, 1.0f,  // Top-right
    -1.0f,  1.0f, 0.0f,  0.0f, 1.0f   // Top-left
};

// On-screen positions of the corners of the color map rectangle
const std::vector<float> COLORMAP_VERTICES = {
    // Positions           // Texture Coords
    -0.95f, -0.95f, 0.0f,  0.0f, 0.0f,  // Bottom-left
    -0.75f, -0.95f, 0.0f,  1.0f, 0.0f,  // Bottom-right
    -0.75f, -0.90f, 0.0f,  1.0f, 1.0f,  // Top-right
    -0.95f, -0.90f, 0.0f,  0.0f, 1.0f   // Top-left
};

//===========================================================================



//=============================OpenGL Variables==============================

GLuint dotVAO, dotVBO;
GLuint wireframeVAO, wireframeVBO, wireframeEBO;
GLuint triangleVAO, triangleVBO, triangleEBO;
GLuint raytracingVAO, raytracingVBO, raytracingEBO;
GLuint lightUBO;
GLuint lightVAO, lightVBO;
GLuint quadVAO, quadVBO, quadEBO;
GLuint colormapVAO, colormapVBO, colormapEBO;
GLuint edgeDetectionVAO, edgeDetectionVBO, edgeDetectionEBO;
GLuint heightMapTexture, normalMapTexture, colorMapTexture, edgeMapTexture;

int dotAmount, wireframeVerticesAmount, triangleVerticesAmount;
std::vector<glm::vec3> vertices;
std::vector<glm::vec3> normals;
glm::vec3 aabbMin, aabbMax;


// Shaders
Shader lightShader;
Shader dotShader;
Shader quadShader;
Shader triangleShader;
Shader lineShader;
Shader raytracingShader;
Shader gaussianBlurShader;
Shader edgeDetectionShader;
Shader colormapVisualizationShader;

//===========================================================================



//==========================OpenGL Helper Functions==========================

static void initializeShaders() {
    lightShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/light_vertex.glsl")
        .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/light_frag.glsl")
        .build();
    dotShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/dot_vertex.glsl")
        .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/dot_frag.glsl")
        .build();
    quadShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/quad_vertex.glsl")
        .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/quad_frag.glsl")
        .build();
    triangleShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/triangle_vertex.glsl")
        .addStage(GL_GEOMETRY_SHADER, RESOURCE_ROOT "shaders/triangle_geometry.glsl")
        .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/triangle_frag.glsl")
        .build();
    lineShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/triangle_vertex.glsl")
        .addStage(GL_GEOMETRY_SHADER, RESOURCE_ROOT "shaders/line_geometry.glsl")
        .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/dot_frag.glsl")
        .build();
    raytracingShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/raytracing_vertex.glsl")
        .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/raytracing_frag.glsl")
        .build();
    gaussianBlurShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/raytracing_vertex.glsl")
        .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/gaussian_blur.glsl")
        .build();
    edgeDetectionShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/raytracing_vertex.glsl")
        .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/edge_detection_frag.glsl")
        .build();
    colormapVisualizationShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/raytracing_vertex.glsl")
        .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/quad_frag.glsl")
        .build();
}

static GLuint createTexture(const ImageData& data) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, data.width, data.height, 0, GL_RGB, GL_UNSIGNED_SHORT, data.pixels.data());

    glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture
    return texture;
}

static GLuint createNormalTexture(const ImageData& data, const std::vector<glm::vec3>& normalMap) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, data.width, data.height, 0, GL_RGB, GL_FLOAT, normalMap.data());

    glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture
    return texture;
}

static float getHeightFromPixel(Pixel pixel) {
    return 0.299f * static_cast<float>(pixel.R) / 65535.0f
        + 0.587f * static_cast<float>(pixel.G) / 65535.0f
        + 0.114f * static_cast<float>(pixel.B) / 65535.0f;
}

static glm::vec3 getVertexFromPixel(const ImageData& data, int x, int z) {
    const Pixel pixel = data.pixels[z * data.width + x];
    float y = getHeightFromPixel(pixel) * data.renderHeight;
    float xPos = data.renderSize * (static_cast<float>(x) / static_cast<float>(data.width) - 0.5f);
    float zPos = data.renderSize * (static_cast<float>(z) / static_cast<float>(data.height) - 0.5f);
    return glm::vec3(xPos, y, zPos);
}

static void clearAndLoadNewVertices(const std::vector<glm::vec3>& vertices, GLuint* VAO, GLuint* VBO) {
    // Clean previous VAO and VBO
    glDeleteVertexArrays(1, VAO);
    glDeleteBuffers(1, VBO);

    // Generate new VAO and VBO
    glGenVertexArrays(1, VAO);
    glGenBuffers(1, VBO);
    glBindVertexArray(*VAO);

    // Upload all the dot positions to the VBO
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

    // Define the vertex attribute for position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

static void clearAndLoadNewVerticesAndIndices(const std::vector<glm::vec3>& vertices, const std::vector<unsigned int>& indices, GLuint* VAO, GLuint* VBO, GLuint* EBO) {
    // Clean previous VAO, VBO and EBO
    glDeleteVertexArrays(1, VAO);
    glDeleteBuffers(1, VBO);
    glDeleteBuffers(1, EBO);

    // Generate new VAO, VBO and EBO
    glGenVertexArrays(1, VAO);
    glGenBuffers(1, VBO);
    glGenBuffers(1, EBO);
    glBindVertexArray(*VAO);

    // Upload all the vertex positions to the VBO
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

    // Upload all the vertex indices to the EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Define the vertex attribute for position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

//===========================================================================

