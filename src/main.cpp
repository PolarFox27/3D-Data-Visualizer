#include <gui.h>

const int WIDTH = 1200;
const int HEIGHT = 800;

//============================Vertices Functions=============================

static float heightFromPixel(Pixel pixel) {
    return 0.299f * static_cast<float>(pixel.R) / 255.0f
         + 0.587f * static_cast<float>(pixel.G) / 255.0f
         + 0.114f * static_cast<float>(pixel.B) / 255.0f;
}

static glm::vec3 getVertexFromPixel(const ImageData& data, int x, int z) {
    const Pixel pixel = data.pixels[z * data.width + x];
    float y = heightFromPixel(pixel) * data.render_height;
    float xPos = data.render_size * (static_cast<float>(x) / static_cast<float>(data.width) - 0.5f);
    float zPos = data.render_size * (static_cast<float>(z) / static_cast<float>(data.height) - 0.5f);
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


static std::vector<glm::vec3> loadVertices(ImageData& data, float* maxHeight, float* minHeight) {
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<glm::vec3> vertices;
    vertices.reserve(data.width * data.height * 2);
    *minHeight = data.render_size;
    *maxHeight = 0.0f;

    // Compute all vertices
    for (int z = 0; z < data.height; ++z) {
        for (int x = 0; x < data.width; ++x) {
            const glm::vec3 vertex = getVertexFromPixel(data, x, z);
            if (vertex.y < *minHeight) *minHeight = vertex.y;
            else if (vertex.y > *maxHeight) *maxHeight = vertex.y;
            vertices.push_back(vertex);
        }
    }

    // Add base vertices for lines
    for (int i = 0; i < data.height * data.width; i++) {
        glm::vec3 v = vertices[i];
        vertices.push_back(glm::vec3(v.x, 0.0f, v.z));
    }

    auto end = std::chrono::high_resolution_clock::now();
    printElapsedTime(start, end, "Creation of data points");
    return vertices;
}

static void loadDotVertices(const std::vector<glm::vec3>& vertices, GLuint* dotVAO, GLuint* dotVBO) {
    auto start = std::chrono::high_resolution_clock::now();
    clearAndLoadNewVertices(vertices, dotVAO, dotVBO);
    auto end = std::chrono::high_resolution_clock::now();
    printElapsedTime(start, end, "Loading of dot vertices");
    return;
}

static int loadLinesVertices(int width, int height, const std::vector<glm::vec3>& vertices, GLuint* lineVAO, GLuint* lineVBO, GLuint* lineEBO) {
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<unsigned int> indices;
    int amount = width * height;
    indices.reserve(amount * 2);

    for (int i = 0; i < amount; i++) {

        indices.push_back(i);
        indices.push_back(i + amount);
    }

    clearAndLoadNewVerticesAndEBO(vertices, indices, lineVAO, lineVBO, lineEBO);
    auto end = std::chrono::high_resolution_clock::now();
    printElapsedTime(start, end, "Loading of line vertices");
    return indices.size();
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

static int loadQuadVertices(const ImageData& data, GLuint* quadVAO, GLuint* quadVBO, GLuint* quadEBO) {
    float offsetX = -0.5f * data.render_size / static_cast<float>(data.width);
    float offsetZ = -0.5f * data.render_size / static_cast<float>(data.height);
    float vertices[] = {
        // Positions                                                                // Texture Coords
        -0.5f*data.render_size + offsetX, 0.0f, -0.5f * data.render_size + offsetZ,  0.0f, 0.0f, // Bottom-left
         0.5f * data.render_size + offsetX, 0.0f, -0.5f * data.render_size + offsetZ,  1.0f, 0.0f, // Bottom-right
         0.5f * data.render_size + offsetX, 0.0f,  0.5f * data.render_size + offsetZ,  1.0f, 1.0f, // Top-right
        -0.5f * data.render_size + offsetX, 0.0f, 0.5f * data.render_size + offsetZ,  0.0f, 1.0f  // Top-left
    };
    unsigned int indices[] = {
        0, 1, 2,  // First Triangle
        2, 3, 0   // Second Triangle
    };
    
    // Clean previous VAO and VBO
    glDeleteVertexArrays(1, quadVAO);
    glDeleteBuffers(1, quadVBO);
    glDeleteBuffers(1, quadEBO);

    // Generate new VAO and VBO
    glGenVertexArrays(1, quadVAO);
    glGenBuffers(1, quadVBO);
    glGenBuffers(1, quadEBO);

    // Bind VAO
    glBindVertexArray(*quadVAO);

    // Bind and set VBO data
    glBindBuffer(GL_ARRAY_BUFFER, *quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Bind and set EBO data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *quadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Vertex attribute: positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Vertex attribute: texture coordinates
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0); // Unbind VAO
    return 6;
}

//===========================================================================


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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, data.width, data.height, 0, GL_RGB, GL_UNSIGNED_BYTE, data.pixels.data());

    glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture
    return texture;
}


// Program entry point. Everything starts here.
int main(int argc, char** argv)
{
    // Create program window
    Window w{ "3D Data Visualizer", glm::ivec2(WIDTH, HEIGHT), OpenGLVersion::GL41 };
    WINDOW = &w;
    glEnable(GL_DEPTH);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_DEBUG_OUTPUT);

    // Parse initial scene config TOML
    Trackball t = readInitialConfig(WINDOW, image_data, lights);
    TRACKBALL = &t;
    render_height = image_data.render_height;
    render_size = image_data.render_size;
    max_render_distance = render_size;

    WINDOW->registerKeyCallback(keyPressedHandler);

    // Create dot cloud + lines vertices + wireframe vertices
    GLuint dotVAO, dotVBO;
    GLuint lineVAO, lineVBO, lineEBO;
    GLuint wireframeVAO, wireframeVBO, wireframeEBO;
    float minHeight, maxHeight;
    std::vector<glm::vec3> vertices = loadVertices(image_data, &maxHeight, &minHeight);
    int dotAmount = image_data.width * image_data.height;
    loadDotVertices(vertices, &dotVAO, &dotVBO);
    loadLinesVertices(image_data.width, image_data.height, vertices, &lineVAO, &lineVBO, &lineEBO);
    int wireframeVerticesAmount = loadWireframeVertices(image_data.width, image_data.height, vertices, &wireframeVAO, &wireframeVBO, &wireframeEBO);

    // Create triangle vertices
    GLuint triangleVAO, triangleVBO, triangleEBO;
    int triangleVerticesAmount = loadTriangleVertices(image_data.width, image_data.height, vertices, &triangleVAO, &triangleVBO, &triangleEBO);

    // Light VAO and VBO
    GLuint lightVAO, lightVBO;
    glGenVertexArrays(1, &lightVAO);
    glGenBuffers(1, &lightVBO);
    glBindVertexArray(lightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lightVBO);

    // Quad Texture and Vertices
    GLuint quadTexture = createTexture(image_data);
    GLuint quadVAO, quadVBO, quadEBO;
    loadQuadVertices(image_data, &quadVAO, &quadVBO, &quadEBO);


    // Shader
    const Shader lightShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/light_vertex.glsl")
                                              .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/light_frag.glsl")
                                              .build();
    const Shader dotShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/dot_vertex.glsl")
                                            .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/dot_frag.glsl")
                                            .build();
    const Shader quadShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/quad_vertex.glsl")
                                             .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/quad_frag.glsl")
                                             .build();
    const Shader triangleShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/triangle_vertex.glsl")
                                                 .addStage(GL_GEOMETRY_SHADER, RESOURCE_ROOT "shaders/triangle_geometry.glsl")
                                                 .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/triangle_frag.glsl")
                                                 .build();


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
        if (render_height != image_data.render_height || render_size != image_data.render_size) {
            image_data.render_height = render_height;
            image_data.render_size = render_size;
            vertices = loadVertices(image_data, &maxHeight, &minHeight);
            loadDotVertices(vertices, &dotVAO, &dotVBO);
            loadLinesVertices(image_data.width, image_data.height, vertices, &lineVAO, &lineVBO, &lineEBO);
            int wireframeVerticesAmount = loadWireframeVertices(image_data.width, image_data.height, vertices, &wireframeVAO, &wireframeVBO, &wireframeEBO);
            int triangleVerticesAmount = loadTriangleVertices(image_data.width, image_data.height, vertices, &triangleVAO, &triangleVBO, &triangleEBO);
            loadQuadVertices(image_data, &quadVAO, &quadVBO, &quadEBO);
        }

        // Compute distance to camera
        const glm::vec3 cameraPos = TRACKBALL->position();

        // Set model/view/projection matrix.
        const glm::mat4 model { 1.0f };
        const glm::mat4 view = TRACKBALL->viewMatrix();
        const glm::mat4 projection = TRACKBALL->projectionMatrix();
        const glm::mat4 mvp = projection * view * model;
        const int mode = static_cast<int>(render_mode);

        // Draw Flat Image
        if (render_quad) {
            quadShader.bind();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, quadTexture);
            glUniform1i(quadShader.getUniformLocation("texture1"), 0); // Pass texture unit 0
            glUniformMatrix4fv(dotShader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));

            // Render the quad
            glBindVertexArray(quadVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
        
        //Shader and variable setup
        dotShader.bind();
        glUniform3fv(dotShader.getUniformLocation("color1"), 1, glm::value_ptr(color_1));
        glUniform3fv(dotShader.getUniformLocation("color2"), 1, glm::value_ptr(color_2));
        glUniform1iv(dotShader.getUniformLocation("mode"), 1, &mode);
        glUniform1f(dotShader.getUniformLocation("minHeight"), minHeight);
        glUniform1f(dotShader.getUniformLocation("maxHeight"), maxHeight);
        glUniform1f(dotShader.getUniformLocation("minDistanceToCamera"), 0.0f);
        glUniform1f(dotShader.getUniformLocation("maxDistanceToCamera"), max_render_distance);
        glUniformMatrix4fv(dotShader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
        glUniform3fv(dotShader.getUniformLocation("cameraPos"), 1, glm::value_ptr(cameraPos));

        // Draw dots
        if (render_dots) {
            
            // Render dots
            glPointSize(dot_size);
            glBindVertexArray(dotVAO);
            glDrawArrays(GL_POINTS, 0, dotAmount);
            
            // Render lines
            if (render_lines) {
                glBindVertexArray(lineVAO);
                glDrawElements(GL_LINES, dotAmount*2, GL_UNSIGNED_INT, 0);
            }

            // Render wireframe
            if (render_wireframe) {
                glBindVertexArray(wireframeVAO);
                glDrawElements(GL_LINES, wireframeVerticesAmount, GL_UNSIGNED_INT, 0);
            }
            glBindVertexArray(0);
        }

        // Draw triangles
        if (render_triangles) {
            triangleShader.bind();
            glUniform3fv(triangleShader.getUniformLocation("color1"), 1, glm::value_ptr(color_1));
            glUniform3fv(triangleShader.getUniformLocation("color2"), 1, glm::value_ptr(color_2));
            glUniform1iv(triangleShader.getUniformLocation("mode"), 1, &mode);
            glUniform1f(triangleShader.getUniformLocation("minHeight"), minHeight);
            glUniform1f(triangleShader.getUniformLocation("maxHeight"), maxHeight);
            glUniform3fv(triangleShader.getUniformLocation("lightDirection"), 1, glm::value_ptr(lights[selectedLightIndex].position));
            glUniform3fv(triangleShader.getUniformLocation("lightColor"), 1, glm::value_ptr(lights[selectedLightIndex].color));
            glUniformMatrix4fv(triangleShader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
            glBindVertexArray(triangleVAO);
            glDrawElements(GL_LINES_ADJACENCY, triangleVerticesAmount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
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

        // Present result to the screen.
        WINDOW->swapBuffers();
    }

    // Cleanup
    glDeleteBuffers(1, &lightVBO);
    glDeleteBuffers(1, &dotVBO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteBuffers(1, &wireframeVBO);
    glDeleteBuffers(1, &triangleVBO);
    glDeleteBuffers(1, &quadEBO);
    glDeleteBuffers(1, &lineEBO);
    glDeleteBuffers(1, &wireframeEBO);
    glDeleteBuffers(1, &triangleEBO);
    glDeleteVertexArrays(1, &lightVAO);
    glDeleteVertexArrays(1, &dotVAO);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteVertexArrays(1, &wireframeVAO);
    glDeleteVertexArrays(1, &triangleVAO);

    return 0;
}

