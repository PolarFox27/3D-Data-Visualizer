#include <gui.h>

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
std::vector<glm::vec4> derivatives;         // stores for each dot : (xPos, zPos, xDerivative, zDerivative)
std::vector<glm::vec4> secondDerivatives;   // stores for each dot: (x'', z'', x'z', z'x')
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


const unsigned int QUAD_INDICES[] = {
        0, 1, 2,  // First Triangle
        2, 3, 0   // Second Triangle
};

const std::vector<float> RAYTRACING_VERTICES = {
    // Positions       // Texture Coords
    -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,  // Bottom-left
     1.0f, -1.0f, 0.0f,  1.0f, 0.0f,  // Bottom-right
     1.0f,  1.0f, 0.0f,  1.0f, 1.0f,  // Top-right
    -1.0f,  1.0f, 0.0f,  0.0f, 1.0f   // Top-left
};

const std::vector<float> COLORMAP_VERTICES = {
    // Positions           // Texture Coords
    -0.95f, -0.95f, 0.0f,  0.0f, 0.0f,  // Bottom-left
    -0.75f, -0.95f, 0.0f,  1.0f, 0.0f,  // Bottom-right
    -0.75f, -0.90f, 0.0f,  1.0f, 1.0f,  // Top-right
    -0.95f, -0.90f, 0.0f,  0.0f, 1.0f   // Top-left
};

//===========================================================================



//============================Vertices Functions=============================

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

static void clearAndLoadNewVerticesAndEBO(const std::vector<glm::vec3>& vertices, const std::vector<unsigned int>& indices, GLuint* VAO, GLuint* VBO, GLuint* EBO) {
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


static std::vector<glm::vec3> loadVertices(const ImageData& data, glm::vec3& aabbMin, glm::vec3& aabbMax) {
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<glm::vec3> vertices;
    vertices.reserve(data.width * data.height * 2);
    float minHeight = data.renderSize;
    float maxHeight = 0.0f;

    // Compute all vertices
    for (int z = 0; z < data.height; ++z) {
        for (int x = 0; x < data.width; ++x) {
            const glm::vec3 vertex = getVertexFromPixel(data, x, z);
            if (vertex.y < minHeight) minHeight = vertex.y;
            else if (vertex.y > maxHeight) maxHeight = vertex.y;
            vertices.push_back(vertex);
        }
    }

    // Add base vertices for lines
    for (int i = 0; i < data.height * data.width; i++) {
        glm::vec3 v = vertices[i];
        vertices.push_back(glm::vec3(v.x, 0.0f, v.z));
    }

    aabbMin = glm::vec3(-data.renderSize / 2.0f, minHeight, -data.renderSize / 2.0f);
    aabbMax = glm::vec3(data.renderSize / 2.0f, maxHeight, data.renderSize / 2.0f);

    auto end = std::chrono::high_resolution_clock::now();
    printElapsedTime(start, end, "Creation of data points");
    return vertices;
}

static std::vector<glm::vec3> computeNormalMap(const ImageData& data, const std::vector<glm::vec3>& vertices, std::vector<glm::vec4>& derivatives) {
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<glm::vec3> normalMap;
    normalMap.reserve(data.width * data.height);
    derivatives = std::vector<glm::vec4>{};
    derivatives.reserve(data.width * data.height);
    for (int z = 0; z < data.height; ++z) {
        for (int x = 0; x < data.width; ++x) {
            const glm::vec3 current = getVertexFromPixel(data, x, z);
            const glm::vec3 x1 = x > 0 ? getVertexFromPixel(data, x - 1, z) : getVertexFromPixel(data, x, z);
            const glm::vec3 x2 = (x+1) < data.width ? getVertexFromPixel(data, x + 1, z) : getVertexFromPixel(data, x, z);
            const glm::vec3 z1 = z > 0 ? getVertexFromPixel(data, x, z - 1) : getVertexFromPixel(data, x, z);
            const glm::vec3 z2 = (z + 1) < data.height ? getVertexFromPixel(data, x, z + 1) : getVertexFromPixel(data, x, z);
            
            float xDerivative = (x2.y - x1.y) / (x2.x - x1.x);
            float zDerivative = (z2.y - z1.y) / (z2.z - z1.z);
            normalMap.push_back(glm::normalize(glm::vec3(-xDerivative, 1, -zDerivative)));
            derivatives.push_back(glm::vec4(current.x, current.z, xDerivative, zDerivative));
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    printElapsedTime(start, end, "Creation of normal map");
    return normalMap;
}

static std::vector<glm::vec4> computeSecondDerivativeMap(const ImageData& data, const std::vector<glm::vec4>& derivatives) {
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<glm::vec4> secondDerivatives;
    secondDerivatives.reserve(data.width * data.height);
    for (int z = 0; z < data.height; ++z) {
        for (int x = 0; x < data.width; ++x) {
            const glm::vec4 x1 = x > 0 ? derivatives[z * data.width + x - 1] : derivatives[z * data.width + x];
            const glm::vec4 x2 = (x + 1) < data.width ? derivatives[z * data.width + x + 1] : derivatives[z * data.width + x];
            const glm::vec4 z1 = z > 0 ? derivatives[(z-1) * data.width + x] : derivatives[z * data.width + x];
            const glm::vec4 z2 = (z + 1) < data.height ? derivatives[(z+1) * data.width + x] : derivatives[z * data.width + x];

            float xxDerivative = (x2.z - x1.z) / (x2.x - x1.x);
            float zzDerivative = (z2.w - z1.w) / (z2.y - z1.y);
            float xzDerivative = (z2.z - z1.z) / (z2.y - z1.y);
            float zxDerivative = (x2.w - x1.w) / (x2.y - x1.y);
            secondDerivatives.push_back(glm::vec4(xxDerivative, zzDerivative, xzDerivative, zxDerivative));
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    printElapsedTime(start, end, "Creation of second derivatives map");
    return secondDerivatives;
}

static void loadDotVertices(const std::vector<glm::vec3>& vertices, GLuint* dotVAO, GLuint* dotVBO) {
    auto start = std::chrono::high_resolution_clock::now();
    clearAndLoadNewVertices(vertices, dotVAO, dotVBO);
    auto end = std::chrono::high_resolution_clock::now();
    printElapsedTime(start, end, "Loading of dot vertices");
    return;
}

static int loadWireframeVertices(int width, int height, const std::vector<glm::vec3>& vertices, GLuint* wireframeVAO, GLuint* wireframeVBO, GLuint* wireframeEBO) {
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<unsigned int> indices;
    int amount = width * height;
    indices.reserve(amount*2);

    for (int i = 0; i < amount; i++) {

        if ((i + 1) % width > 0) {
            indices.push_back(i);
            indices.push_back(i+1);
        }

        if (i + width < amount) {
            indices.push_back(i);
            indices.push_back(i + width);
        }
    }
    clearAndLoadNewVerticesAndEBO(vertices, indices, wireframeVAO, wireframeVBO, wireframeEBO);
    auto end = std::chrono::high_resolution_clock::now();
    printElapsedTime(start, end, "Loading of wireframe vertices");
    return indices.size();
}

static int loadTriangleVertices(int width, int height, const std::vector<glm::vec3>& vertices, GLuint* triangleVAO, GLuint* triangleVBO, GLuint* triangleEBO) {

    auto start = std::chrono::high_resolution_clock::now();
    std::vector<unsigned int> indices;
    indices.reserve(width * height * 4);

    for (int z = 0; z < height-1; ++z) {
        for (int x = 0; x < width-1; ++x) {
            int bottom_left = z * width + x;
            int top_left = (z+1) * width + x;
            int top_right = (z + 1) * width + x + 1;
            int bottom_right = z * width + x+1;
            indices.push_back(bottom_left);
            indices.push_back(top_left);
            indices.push_back(top_right);
            indices.push_back(bottom_right);
        }
    }
    clearAndLoadNewVerticesAndEBO(vertices, indices, triangleVAO, triangleVBO, triangleEBO);
    auto end = std::chrono::high_resolution_clock::now();
    printElapsedTime(start, end, "Loading of triangle vertices");
    return indices.size();
}


static void loadQuadVertices(GLuint& VAO, GLuint& VBO, GLuint& EBO, const std::vector<float>& vertices) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // Set up VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    // Set up EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(QUAD_INDICES), QUAD_INDICES, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

static void loadFlatQuadVertices(const ImageData& data, GLuint& quadVAO, GLuint& quadVBO, GLuint& quadEBO) {
    float offsetX = -0.5f * data.renderSize / static_cast<float>(data.width);
    float offsetZ = -0.5f * data.renderSize / static_cast<float>(data.height);
    std::vector<float> vertices = {
        // Positions                                                                // Texture Coords
        -0.5f * data.renderSize + offsetX, 0.0f, -0.5f * data.renderSize + offsetZ,  0.0f, 0.0f, // Bottom-left
         0.5f * data.renderSize + offsetX, 0.0f, -0.5f * data.renderSize + offsetZ,  1.0f, 0.0f, // Bottom-right
         0.5f * data.renderSize + offsetX, 0.0f,  0.5f * data.renderSize + offsetZ,  1.0f, 1.0f, // Top-right
        -0.5f * data.renderSize + offsetX, 0.0f, 0.5f * data.renderSize + offsetZ,  0.0f, 1.0f  // Top-left
    };
    loadQuadVertices(quadVAO, quadVBO, quadEBO, vertices);
}

static void loadLightsToUBO(const std::vector<Light>& lightArray, GLuint UBO) {
    // Arrange data in the correct format
    glBindBuffer(GL_UNIFORM_BUFFER, UBO);
    glm::vec4 lightData[2 * MAX_LIGHT_AMOUNT]{};

    for (int i = 0; i < lightArray.size(); i++) {
        lightData[i] = glm::vec4(lightArray[i].position, 1.0f);
        lightData[MAX_LIGHT_AMOUNT + i] = glm::vec4(lightArray[i].color, 1.0f);
    }

    // Load data to the UBO
    glBufferSubData(GL_UNIFORM_BUFFER, 0, 2 * MAX_LIGHT_AMOUNT * sizeof(glm::vec4), lightData);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

static GLuint createEdgeMapTexture(const ImageData& data, const int smallGaussianFilterSize, const int largeGaussianFilterSize) {
    auto start = std::chrono::high_resolution_clock::now();
    GLuint pingpongFBO[5], pingpongTextures[5];
    glm::vec2 horizontalDir(1.0f / static_cast<float>(data.width), 0.0f), verticalDir(0.0f, 1.0f / static_cast<float>(data.height));
    glGenFramebuffers(5, pingpongFBO);
    glGenTextures(5, pingpongTextures);
    glViewport(0, 0, data.width, data.height);
    
    // Creation of 5 frame buffers
    for (int i = 0; i < 5; i++) {
        glBindTexture(GL_TEXTURE_2D, pingpongTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, data.width, data.height, 0, GL_RGB, GL_UNSIGNED_SHORT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongTextures[i], 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // 2 Gaussian filters in 2 passes
    for (int i = 0; i < 4; i++) {
        GLuint texture = (i % 2 == 0) ? heightMapTexture : pingpongTextures[i - 1];
        glm::vec2 direction = (i % 2 == 0) ? horizontalDir : verticalDir;
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        gaussianBlurShader.bind();
        glUniform1i(gaussianBlurShader.getUniformLocation("inputTexture"), 0);
        glUniform2f(gaussianBlurShader.getUniformLocation("direction"), direction.x, direction.y);
        glUniform1i(gaussianBlurShader.getUniformLocation("size"), (i/2 == 0) ? smallGaussianFilterSize : largeGaussianFilterSize);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(raytracingVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    // Final edge map texture
    glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[4]);
    edgeDetectionShader.bind();
    glUniform1i(edgeDetectionShader.getUniformLocation("originalBlurredTexture"), 0);
    glUniform1i(edgeDetectionShader.getUniformLocation("largeBlurredTexture"), 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pingpongTextures[1]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, pingpongTextures[3]);
    glBindVertexArray(raytracingVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Create final texture from the last frame buffer
    GLuint finalTexture;
    glGenTextures(1, &finalTexture);
    glBindTexture(GL_TEXTURE_2D, finalTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, data.width, data.height, 0, GL_RGB, GL_UNSIGNED_SHORT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[4]);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, data.width, data.height, 0);

    // Cleanup
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, WIDTH, HEIGHT);
    auto end = std::chrono::high_resolution_clock::now();
    printElapsedTime(start, end, "Edge Map Creation");
    return finalTexture;
}

//===========================================================================



static void loadNextColorMap() {
    activeColorMap = (activeColorMap + 1) % colorMaps.size();
    colorMapTexture = createTexture(colorMaps[activeColorMap]);
}


// Key Pressed Handler
static void keyPressedHandler(int key, int /* scancode */, int action, int /* mods */) {
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        showGui = !showGui;
    }

    if (action != GLFW_RELEASE)
        return;

    const bool shiftPressed = WINDOW->isKeyPressed(GLFW_KEY_LEFT_SHIFT) || WINDOW->isKeyPressed(GLFW_KEY_RIGHT_SHIFT);

    switch (key) {
    case GLFW_KEY_H: {
        printHelp();
        return;
    }
    case GLFW_KEY_L: {
        if (shiftPressed)
            lights.push_back(Light{ TRACKBALL->position(), glm::vec3(1) });
        else
            lights[selectedLightIndex].position = TRACKBALL->position();
        return;
    }
    case GLFW_KEY_UP: {
        selectPreviousLight();
        return;
    }
    case GLFW_KEY_DOWN: {
        selectNextLight();
        return;
    }
    case GLFW_KEY_N: {
        resetLights();
        return;
    }
    case GLFW_KEY_DELETE: {
        deleteLight();
        return;
    }
    case GLFW_KEY_C: {
        loadNextColorMap();
        return;
    }
    case GLFW_KEY_R: {
        if (shiftPressed) { // If shift pressed, decrease selected light red channel by 0.1
            if (lights[selectedLightIndex].color.x >= 0.1f)
                lights[selectedLightIndex].color.x -= 0.1f;
        }
        else { // Else, increase selected light red channel by 0.1
            if (lights[selectedLightIndex].color.x <= 0.9f)
                lights[selectedLightIndex].color.x += 0.1f;
        }
        std::cout << "Light " << selectedLightIndex << " color : [" << lights[selectedLightIndex].color.x << ", "
            << lights[selectedLightIndex].color.y << ", "
            << lights[selectedLightIndex].color.z << "]"
            << std::endl;
        return;
    }
    case GLFW_KEY_G: {
        if (shiftPressed) { // If shift pressed, decrease selected light green channel by 0.1
            if (lights[selectedLightIndex].color.y >= 0.1f)
                lights[selectedLightIndex].color.y -= 0.1f;
        }
        else { // Else, increase selected light green channel by 0.1
            if (lights[selectedLightIndex].color.y <= 0.9f)
                lights[selectedLightIndex].color.y += 0.1f;
        }
        std::cout << "Light " << selectedLightIndex << " color : [" << lights[selectedLightIndex].color.x << ", "
            << lights[selectedLightIndex].color.y << ", "
            << lights[selectedLightIndex].color.z << "]"
            << std::endl;
        return;
    }
    case GLFW_KEY_B: {
        if (shiftPressed) { // If shift pressed, decrease selected light blue channel by 0.1
            if (lights[selectedLightIndex].color.z >= 0.1f)
                lights[selectedLightIndex].color.z -= 0.1f;
        }
        else { // Else, increase selected light blue channel by 0.1
            if (lights[selectedLightIndex].color.z <= 0.9f)
                lights[selectedLightIndex].color.z += 0.1f;
        }
        std::cout << "Light " << selectedLightIndex << " color : [" << lights[selectedLightIndex].color.x << ", "
            << lights[selectedLightIndex].color.y << ", "
            << lights[selectedLightIndex].color.z << "]"
            << std::endl;
        return;
    }
    default:
        return;
    };
}


// Program entry point. Everything starts here.
int main(int argc, char** argv)
{
    // Create program window
    Window w{ "3D Data Visualizer", glm::ivec2(WIDTH, HEIGHT), OpenGLVersion::GL41 };
    WINDOW = &w;
    Trackball t{ WINDOW, glm::radians(50.0) };
    TRACKBALL = &t;
    glEnable(GL_DEPTH);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_DEBUG_OUTPUT);

    // Parse initial scene config TOML
    readInitialConfig(TRACKBALL, imageData, colorMaps, lights);
    renderHeight = imageData.renderHeight;
    renderSize = imageData.renderSize;
    int largeBlurSize = edgeLargeBlurSize, smallBlurSize = edgeSmallBlurSize;
    maxRenderDistance = renderSize;
    WINDOW->registerKeyCallback(keyPressedHandler);


    // Shaders
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


    // Create color map texture
    colorMapTexture = createTexture(colorMaps[activeColorMap]);

    // Quad Texture and Vertices
    heightMapTexture = createTexture(imageData);
    loadFlatQuadVertices(imageData, quadVAO, quadVBO, quadEBO);

    // RayTracing
    loadQuadVertices(raytracingVAO, raytracingVBO, raytracingEBO, RAYTRACING_VERTICES);
    loadQuadVertices(colormapVAO, colormapVBO, colormapEBO, COLORMAP_VERTICES);

    // Create dot cloud + wireframe vertices
    vertices = loadVertices(imageData, aabbMin, aabbMax);
    dotAmount = imageData.width * imageData.height;
    loadDotVertices(vertices, &dotVAO, &dotVBO);
    wireframeVerticesAmount = loadWireframeVertices(imageData.width, imageData.height, vertices, &wireframeVAO, &wireframeVBO, &wireframeEBO);

    // Create triangle vertices
    triangleVerticesAmount = loadTriangleVertices(imageData.width, imageData.height, vertices, &triangleVAO, &triangleVBO, &triangleEBO);

    // Compute normal and edge maps
    normals = computeNormalMap(imageData, vertices, derivatives);
    normalMapTexture = createNormalTexture(imageData, normals);
    edgeMapTexture = createEdgeMapTexture(imageData, smallBlurSize, largeBlurSize);


    // Light VAO and VBO
    glGenVertexArrays(1, &lightVAO);
    glGenBuffers(1, &lightVBO);
    glBindVertexArray(lightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lightVBO);

    // Light UBO
    glGenBuffers(1, &lightUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, lightUBO);

    // Allocate memory for 20 Lights (with padding so vec4 is used)
    glBufferData(GL_UNIFORM_BUFFER, 2 * MAX_LIGHT_AMOUNT * sizeof(glm::vec4), nullptr, GL_DYNAMIC_DRAW);

    // Bind the UBO to binding point 0
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, lightUBO);


    // Main loop.
    while (!WINDOW->shouldClose()) {
        // Update input and UI
        WINDOW->updateInput();
        renderGUI();
        
        // Clear the framebuffer to black and depth to maximum value (ranges from [-1.0 to +1.0]).
        glViewport(0, 0, WINDOW->getWindowSize().x, WINDOW->getWindowSize().y);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Check if render dimensions changed and update vertices
        if (renderHeight != imageData.renderHeight || renderSize != imageData.renderSize) {
            imageData.renderHeight = renderHeight;
            imageData.renderSize = renderSize;
            vertices = loadVertices(imageData, aabbMin, aabbMax);
            normals = computeNormalMap(imageData, vertices, derivatives);
            normalMapTexture = createNormalTexture(imageData, normals);
            edgeMapTexture = createEdgeMapTexture(imageData, smallBlurSize, largeBlurSize);
            loadDotVertices(vertices, &dotVAO, &dotVBO);
            wireframeVerticesAmount = loadWireframeVertices(imageData.width, imageData.height, vertices, &wireframeVAO, &wireframeVBO, &wireframeEBO);
            triangleVerticesAmount = loadTriangleVertices(imageData.width, imageData.height, vertices, &triangleVAO, &triangleVBO, &triangleEBO);
            loadFlatQuadVertices(imageData, quadVAO, quadVBO, quadEBO);
        }

        if (smallBlurSize != edgeSmallBlurSize || largeBlurSize != edgeLargeBlurSize) {
            smallBlurSize = edgeSmallBlurSize;
            largeBlurSize = edgeLargeBlurSize;
            edgeMapTexture = createEdgeMapTexture(imageData, smallBlurSize, largeBlurSize);
        }
      
        // Compute distance to camera
        const glm::vec3 cameraPos = TRACKBALL->position();

        // Set model/view/projection matrix.
        const glm::mat4 model { 1.0f };
        const glm::mat4 view = TRACKBALL->viewMatrix();
        const glm::mat4 projection = TRACKBALL->projectionMatrix();
        const glm::mat4 mvp = projection * view * model;
        const int mode = static_cast<int>(renderMode);

        loadLightsToUBO(lights, lightUBO);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, heightMapTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normalMapTexture);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, colorMapTexture);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, edgeMapTexture);

        // Ray Tracing
        if (showRaytracingTab) {
            if (showRaytracing) {
                raytracingShader.bind();
                glUniform1i(raytracingShader.getUniformLocation("heightMap"), 0); // Pass texture unit 0
                glUniform1i(raytracingShader.getUniformLocation("normalMap"), 1); // Pass texture unit 1
                glUniform1i(raytracingShader.getUniformLocation("colorMap"), 2); // Pass texture unit 2
                glUniform1i(raytracingShader.getUniformLocation("edgeMap"), 3); // Pass texture unit 3
                glUniformMatrix4fv(raytracingShader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
                glUniform1iv(raytracingShader.getUniformLocation("maxSteps"), 1, &maxSteps);
                glUniform3fv(raytracingShader.getUniformLocation("aabbMin"), 1, glm::value_ptr(aabbMin));
                glUniform3fv(raytracingShader.getUniformLocation("aabbMax"), 1, glm::value_ptr(aabbMax));
                glUniform1f(raytracingShader.getUniformLocation("renderHeight"), imageData.renderHeight);
                glUniform1i(raytracingShader.getUniformLocation("lightAmount"), lights.size());
                glUniform1iv(raytracingShader.getUniformLocation("mode"), 1, &mode);
                glUniform1i(raytracingShader.getUniformLocation("binarySearch"), useBinarySearch);
                raytracingShader.bindUniformBlock("LightData", 0, lightUBO);

                // Render
                glBindVertexArray(raytracingVAO);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
            }
        }

        else {
            // Draw Flat Image
            if (showFlatQuad) {
                quadShader.bind();
                glUniform1i(quadShader.getUniformLocation("inputTexture"), 0); // Pass texture unit 0
                glUniformMatrix4fv(dotShader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
                glUniform1f(quadShader.getUniformLocation("height"), height);

                // Render the quad
                glBindVertexArray(quadVAO);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
            }

            //Shader and variable setup
            dotShader.bind();
            glUniform1i(dotShader.getUniformLocation("colorMap"), 2); // Pass texture unit 2
            glUniform1iv(dotShader.getUniformLocation("mode"), 1, &mode);
            glUniform1f(dotShader.getUniformLocation("renderHeight"), imageData.renderHeight);
            glUniform1f(dotShader.getUniformLocation("minDistanceToCamera"), 0.0f);
            glUniform1f(dotShader.getUniformLocation("maxDistanceToCamera"), maxRenderDistance);
            glUniformMatrix4fv(dotShader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
            glUniform3fv(dotShader.getUniformLocation("cameraPos"), 1, glm::value_ptr(cameraPos));

            // Draw dots
            if (showDots) {

                // Render dots
                glPointSize(dotSize);
                glBindVertexArray(dotVAO);
                glDrawArrays(GL_POINTS, 0, dotAmount);

                // Render lines
                if (showLines) {
                    lineShader.bind();
                    glUniform1i(lineShader.getUniformLocation("colorMap"), 2); // Pass texture unit 2
                    glUniform1iv(lineShader.getUniformLocation("mode"), 1, &mode);
                    glUniform1f(lineShader.getUniformLocation("renderHeight"), imageData.renderHeight);
                    glUniform1f(lineShader.getUniformLocation("minDistanceToCamera"), 0.0f);
                    glUniform1f(lineShader.getUniformLocation("maxDistanceToCamera"), maxRenderDistance);
                    glUniformMatrix4fv(lineShader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
                    glUniform3fv(lineShader.getUniformLocation("cameraPos"), 1, glm::value_ptr(cameraPos));
                    glDrawArrays(GL_POINTS, 0, dotAmount);
                }

                // Render wireframe
                if (showWireframe) {
                    glBindVertexArray(wireframeVAO);
                    glDrawElements(GL_LINES, wireframeVerticesAmount, GL_UNSIGNED_INT, 0);
                }
                glBindVertexArray(0);
            }

            // Draw triangles
            if (showTriangles) {
                triangleShader.bind();
                glUniform1i(triangleShader.getUniformLocation("normalMap"), 1); // Pass texture unit 1
                glUniform1i(triangleShader.getUniformLocation("colorMap"), 2); // Pass texture unit 2
                glUniform1i(triangleShader.getUniformLocation("edgeMap"), 3); // Pass texture unit 3
                glUniform1f(triangleShader.getUniformLocation("renderSize"), imageData.renderSize);
                glUniform1iv(triangleShader.getUniformLocation("mode"), 1, &mode);
                glUniform1f(triangleShader.getUniformLocation("renderHeight"), imageData.renderHeight);
                glUniform1i(triangleShader.getUniformLocation("lightAmount"), lights.size());
                glUniformMatrix4fv(triangleShader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));

                triangleShader.bindUniformBlock("LightData", 0, lightUBO);

                glBindVertexArray(triangleVAO);
                glDrawElements(GL_LINES_ADJACENCY, triangleVerticesAmount, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
            }
        }

        // Draw lights as (square) points. The selected light is bigger
        lightShader.bind();
        {
            const glm::vec4 screenPos = mvp * glm::vec4(lights[selectedLightIndex].position, 1.0f);

            glPointSize(30.0f);
            glUniform4fv(lightShader.getUniformLocation("pos"), 1, glm::value_ptr(screenPos));
            glUniform3fv(lightShader.getUniformLocation("color"), 1, glm::value_ptr(lights[selectedLightIndex].color));
            glBindVertexArray(lightVAO);
            glDrawArrays(GL_POINTS, 0, 1);
            glBindVertexArray(0);
        }
        for (const Light& light : lights) {
            const glm::vec4 screenPos = mvp * glm::vec4(light.position, 1.0f);
            // const glm::vec3 color { 1, 0, 0 };

            glPointSize(10.0f);
            glUniform4fv(lightShader.getUniformLocation("pos"), 1, glm::value_ptr(screenPos));
            glUniform3fv(lightShader.getUniformLocation("color"), 1, glm::value_ptr(light.color));
            glBindVertexArray(lightVAO);
            glDrawArrays(GL_POINTS, 0, 1);
            glBindVertexArray(0);

        }

        //Show Colormap visualization
        if (showColorMap) {
            colormapVisualizationShader.bind();
            glUniform1i(colormapVisualizationShader.getUniformLocation("inputTexture"), 2); // Pass texture unit 2 : colormap

            // Render the quad
            glBindVertexArray(colormapVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        // Present result to the screen.
        WINDOW->swapBuffers();
    }

    // Cleanup
    glDeleteBuffers(1, &lightVBO);
    glDeleteBuffers(1, &dotVBO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteBuffers(1, &wireframeVBO);
    glDeleteBuffers(1, &triangleVBO);
    glDeleteBuffers(1, &raytracingVBO);
    glDeleteBuffers(1, &quadEBO);
    glDeleteBuffers(1, &wireframeEBO);
    glDeleteBuffers(1, &triangleEBO);
    glDeleteBuffers(1, &raytracingEBO);
    glDeleteVertexArrays(1, &lightVAO);
    glDeleteVertexArrays(1, &dotVAO);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteVertexArrays(1, &wireframeVAO);
    glDeleteVertexArrays(1, &triangleVAO);
    glDeleteVertexArrays(1, &raytracingVAO);
    glDeleteBuffers(1, &lightUBO);

    return 0;
}

