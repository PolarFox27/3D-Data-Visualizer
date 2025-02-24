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

/**
 * @brief Initializes all the shader programs.
 * 
 * This function initializes all the shader programs by specifying their stages.
 * The resulting programs are stored in the shader variables.
 */
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

/**
 * @brief Creates an OpenGL texture from an image.
 * 
 * This function creates an OpenGL 2D texture representing the image given.
 * 
 * @param data Image that will be represented by the texture.
 * @return ID of the generated texture.
 */
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

/**
 * @brief Creates an OpenGL texture from a normal map.
 *
 * This function creates an OpenGL 2D texture representing a normal map.
 * The normal map is given as a list of normal vectors. These are encoded as pixel values.
 * It uses a reference to the heightmap to determine the width and height of the normal map.
 *
 * @param data Image that is used to find the width and height of the normal map.
 * @param normalMap Array of normal vectors, representing the normals for each pixel.
 * @return ID of the generated normal map texture.
 */
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

/**
 * @brief Computes the relative height of a pixel.
 * 
 * This function computes the luminance of a pixel, which is used to determine the height the pixel should be rendered at in the cloud dot.
 * It outputs the luminance as a number between 0 (for black) and 1. (for white)
 * 
 * @param pixel Pixel value for which we compute the luminance.
 * @return Luminance of the pixel in the range [0, 1]
 */
static float getHeightFromPixel(Pixel pixel) {
    return 0.299f * static_cast<float>(pixel.R) / 65535.0f
        + 0.587f * static_cast<float>(pixel.G) / 65535.0f
        + 0.114f * static_cast<float>(pixel.B) / 65535.0f;
}

/**
 * @brief Computes the dot position from a heightmap pixel.
 * 
 * This function takes in a pixel position in the heightmap and computes where the corresponding vertex should be in the dot cloud.
 * It is based on the render dimensions of the heightmap, and the luminance of the pixel.
 * 
 * @param data Heightmap image, containing the pixels and the rendering dimensions.
 * @param x Pixel x position (horizontal îndex in the image)
 * @param z Pixel z position (vertical index in the image)
 * @return Vertex 3D position for the given pixel.
 */
static glm::vec3 getVertexFromPixel(const ImageData& data, int x, int z) {
    const Pixel pixel = data.pixels[z * data.width + x];
    float y = getHeightFromPixel(pixel) * data.renderHeight;
    float xPos = data.renderSize * (static_cast<float>(x) / static_cast<float>(data.width) - 0.5f);
    float zPos = data.renderSize * (static_cast<float>(z) / static_cast<float>(data.height) - 0.5f);
    return glm::vec3(xPos, y, zPos);
}

/**
 * @brief Creates a VAO and VBO for the given vertices.
 * 
 * This function clears the given VAO and VBO (if they exist) then generates new ones.
 * The vertices are then uploaded to the VBO.
 * 
 * @param vertices Vertex array to store in the VBO.
 * @param VAO Reference to a VAO that will store the VAO for the vertices.
 * @param VBO Reference to a VBO that will store the VBO for the vertices.
 */
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

/**
 * @brief Creates a VAO, VBO, EBO for the given vertices and indices.
 *
 * This function clears the given VAO, VBO and EBO (if they exist) then generates new ones.
 * The vertices are then uploaded to the VBO, and the indices are uploaded to the EBO.
 *
 * @param vertices Vertex array to store in the VBO.
 * @param indices Index array to store in the EBO.
 * @param VAO Reference to a VAO that will store the VAO for the vertices.
 * @param VBO Reference to a VBO that will store the VBO for the vertices, containing the vertex data.
 * @param EBO Reference to a EBO that will store the EBO for the vertices, containing the index data.
 */
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

