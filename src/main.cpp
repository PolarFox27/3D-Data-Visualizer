#include <gui.h>



//============================Vertices Functions=============================

/**
 * @brief Creates a list of vertices from an image.
 * 
 * This function creates a list of vertices, where each vertex represents a pixel in the heightmap.
 * It also keeps track of the minimum and maximum value to compute the bounding box of the dot clouds.
 * 
 * @param data Heightmap image.
 * @param aabbMin Reference to a 3D vector storing the minimum position of the bounding box.
 * @param aabbMin Reference to a 3D vector storing the maximum position of the bounding box.
 * @return An array of 3D vertex positions.
 */
static std::vector<glm::vec3> loadVertices(const ImageData& data, glm::vec3& aabbMin, glm::vec3& aabbMax) {
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<glm::vec3> vertices;
    vertices.reserve(data.width * data.height);
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

    aabbMin = glm::vec3(-data.renderSize / 2.0f, minHeight, -data.renderSize / 2.0f);
    aabbMax = glm::vec3(data.renderSize / 2.0f, maxHeight, data.renderSize / 2.0f);

    auto end = std::chrono::high_resolution_clock::now();
    printElapsedTime(start, end, "Creation of data points");
    return vertices;
}


/**
 * @brief Computes the normal map from the vertices.
 * 
 * This function computes the normal map by computing the derivatives in the x and z directions for every vertex.
 * It uses a reference to the heightmap to find the dimensions of the heightmap. (and by extension the normal map)
 * For each vertex, it computes the normal vector and stores it in an array.
 * 
 * @param data Heightmap image.
 * @param vertices Array of 3D vertex positions.
 * @return Array of 3D vectors representing the normal map.
 */
static std::vector<glm::vec3> computeNormalMap(const ImageData& data, const std::vector<glm::vec3>& vertices) {
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<glm::vec3> normalMap;
    normalMap.reserve(data.width * data.height);
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
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    printElapsedTime(start, end, "Creation of normal map");
    return normalMap;
}


/**
 * @brief Computes indices for the wireframe and loads them in a VAO, VBO and EBO.
 * 
 * This function computes the indices needed to render the wireframe based on the vertices array and the dimensions of the heightmap.
 * It then loads the vertices and newly computed indices into a VAO, VBO and EBO.
 * It also computes the amount of indices in the array, it will be used to render the wireframe.
 * 
 * @param width Width of the heightmap.
 * @param height Height of the heightmap.
 * @param vertices Array of vertex positions.
 * @param wireframeVAO Reference to where the VAO will be stored.
 * @param wireframeVBO Reference to where the VBO will be stored.
 * @param wireframeEBO Reference to where the EBO will be stored.
 * @return the size of the array of indices.
 */
static int loadWireframeVertices(int width, int height, const std::vector<glm::vec3>& vertices, GLuint* wireframeVAO, GLuint* wireframeVBO, GLuint* wireframeEBO) {
    /*
    * == Code Snippet 5 ==
    * 1. Create an empty array to store the indices.
    * 2. Add all the indices to form the wireframe.
    * 3. Setup the VAO, VBO and EBO for the vertices and the computed indices.
    * 4. Return the size of the array of indices.
    */
    return 0;
}


/**
 * @brief Computes indices for the triangles and loads them in a VAO, VBO and EBO.
 *
 * This function computes the indices needed to render the triangles based on the vertices array and the dimensions of the heightmap.
 * It then loads the vertices and newly computed indices into a VAO, VBO and EBO.
 * It also computes the amount of indices in the array, it will be used to render the triangles.
 *
 * @param width Width of the heightmap.
 * @param height Height of the heightmap.
 * @param vertices Array of vertex positions.
 * @param wireframeVAO Reference to where the VAO will be stored.
 * @param wireframeVBO Reference to where the VBO will be stored.
 * @param wireframeEBO Reference to where the EBO will be stored.
 * @return the size of the array of indices.
 */
static int loadTriangleVertices(int width, int height, const std::vector<glm::vec3>& vertices,
                                GLuint* triangleVAO, GLuint* triangleVBO, GLuint* triangleEBO) {
    /*
    * == Code Snippet 6 ==
    * 1. Create an empty array to store the indices.
    * 2. Add all the indices to form the triangles.
    * 3. Setup the VAO, VBO and EBO for the vertices and the computed indices.
    * 4. Return the size of the array of indices.
    */
    return 0;
}


/**
 * @brief Setup a VAO, VBO and EBO for a quad.
 * 
 * This function takes in the 4 corners of a quad, each represented by 5 float values:
 *  - 3D world position
 *  - 2D texture coordinate
 * It then stores it as the 6 corners of 2 triangles inside a VAO, VBO and EBO.
 * It uses the QUAD_INDICES array as indices in the EBO.
 * 
 * @param VAO Reference to where the VAO will be stored.
 * @param VBO Reference to where the VBO will be stored.
 * @param EBO Reference to where the EBO will be stored.
 * @param vertices Array of vertex positions, the 4 corners of the quad.
 */
static void loadQuadVertices(GLuint& VAO, GLuint& VBO, GLuint& EBO, const std::vector<float>& vertices) {
    /*
    * == Code Snippet 7 ==
    * 1. Generate and bind new VAO, VBO and EBO.
    * 2. Upload the vertex data to the VBO.
    * 3. Upload the quad index data to the EBO. (QUAD_INDICES)
    * 4. Specify the vertex attribute for the 3D position of the vertex.
    * 5. Specify the vertex attribute for the 2D texture coordinate of the vertex.
    * 6. Unbind the VAO.
    */
}


/**
 * @brief Computes the vertices for a flat image and loads them in a VAO, VBO and EBO.
 *
 * This function computes the 4 corners of the flat quad on the XZ plane.
 * The flat quad has the same width and height as the render dimensions of the image.
 * It then stores it as the 6 corners of 2 triangles inside a VAO, VBO and EBO.
 * It uses the QUAD_INDICES array as indices in the EBO.
 *
 * @param data Heightmap image, also storing the render dimensions.
 * @param VAO Reference to where the VAO will be stored.
 * @param VBO Reference to where the VBO will be stored.
 * @param EBO Reference to where the EBO will be stored.
 */
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

/**
 * @brief Stores the lights into a UBO.
 * 
 * This function arranges the lights positions and colors as the data of a UBO.
 * This UBO can then be used by the shader programs.
 * The UBO stores light as follow:
 *  - first all the light positions aligned as vec4, then all the light colors, also aligned as vec4.
 *  - The array size is thus 2 times MAX_LIGHT_AMOUNT
 * 
 * @param lightArray Array of Light structs, representing the light sources.
 * @param UBO ID of the UBO which will contain the light data.  
 */
static void loadLightsToUBO(const std::vector<Light>& lightArray, GLuint UBO) {
    /*
    * == Code Snippet 8 ==
    * 1. Bind the UBO.
    * 2. Compute the light data as a vec4 array.
    * 3. Upload the light data to the UBO.
    * 4. Unbind the UBO.
    */
}

/**
 * @brief Creates an edge map OpenGL texture of the heightmap using a Difference of Gaussian.
 * 
 * This function computes the edge map of the height map image and returns it as an OpenGL 2D texture.
 * It computes it using a Difference of Gaussian, and texture ping-ponging.
 * 
 * @param data Heightmap image, also storing the dimensions of the image.
 * @param smallGaussianFilterSize Radius of the first Gaussian filter.
 * @param largeGaussianFilterSize Radius of the second Gaussian filter.
 * @return ID of the OpenGL texture created, containing the edge map.
 */
static GLuint createEdgeMapTexture(const ImageData& data, const int smallGaussianFilterSize, const int largeGaussianFilterSize) {
    /*
    * == Code Snippet 9 ==
    * 1. Generate 5 textures and FBOs.
    * 2. Set the viewport size to the heightmap dimensions.
    * 3. For each texture: set its data to zero and setup its parameters for wrapping and scaling.
    * 4. For each FBO: bind it to the corresponding texture.
    * 5. Generate the edge map:
    *  - Use 2 textures and FBOs to apply the small gaussian filter in 2 passes (horizontal then vertical)
    *  - Use 2 textures and FBOs to apply similarly the large gaussian filter
    *  - Use the final FBO and texture to make the edge map
    * 6. Create a new texture and copy the texture data from the final FBO and texture.
    * 7. Unbind the last FBO and delete the FBOs and textures.
    * 8. Set the viewport back to its original size.
    * 9. Return the texture.
    */
    return 0;
}

//===========================================================================


/**
 * @brief Main Function.
 */
int main(int argc, char** argv)
{
    // Create program window
    Window w{ "3D Data Visualizer", glm::ivec2(WIDTH, HEIGHT), OpenGLVersion::GL41 };
    WINDOW = &w;
    Trackball t{ WINDOW, glm::radians(50.0) };
    TRACKBALL = &t;
    WINDOW->registerKeyCallback(keyPressedHandler);
    glEnable(GL_DEPTH);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_DEBUG_OUTPUT);
    initializeShaders();

    // Parse initial scene config TOML
    readInitialConfig(TRACKBALL, imageData, colorMaps, lights);
    renderHeight = imageData.renderHeight;
    renderSize = imageData.renderSize;
    int largeBlurSize = edgeLargeBlurSize, smallBlurSize = edgeSmallBlurSize;
    maxRenderDistance = renderSize;


    // Create color map texture
    colorMapTexture = createTexture(colorMaps[activeColorMap]);

    // Quad Texture and Vertices
    heightMapTexture = createTexture(imageData);
    loadFlatQuadVertices(imageData, quadVAO, quadVBO, quadEBO);

    // RayTracing
    loadQuadVertices(raytracingVAO, raytracingVBO, raytracingEBO, RAYTRACING_VERTICES);
    loadQuadVertices(colormapVAO, colormapVBO, colormapEBO, COLORMAP_VERTICES);

    // Create dot cloud
    vertices = loadVertices(imageData, aabbMin, aabbMax);
    dotAmount = imageData.width * imageData.height;
    clearAndLoadNewVertices(vertices, &dotVAO, &dotVBO);
    
    // Create wireframe
    wireframeVerticesAmount = loadWireframeVertices(imageData.width, imageData.height, vertices, &wireframeVAO, &wireframeVBO, &wireframeEBO);

    // Create triangle vertices
    triangleVerticesAmount = loadTriangleVertices(imageData.width, imageData.height, vertices, &triangleVAO, &triangleVBO, &triangleEBO);

    // Compute normal and edge maps
    normals = computeNormalMap(imageData, vertices);
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
            normals = computeNormalMap(imageData, vertices);
            normalMapTexture = createNormalTexture(imageData, normals);
            edgeMapTexture = createEdgeMapTexture(imageData, smallBlurSize, largeBlurSize);
            clearAndLoadNewVertices(vertices, &dotVAO, &dotVBO);
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
                /*
                * == Code Snippet 10 ==
                * 1. Bind the Raytracing Shader and VAO.
                * 2. Setup all the shader uniform parameters:
                *  - heightMap, normalMap, colorMap, edgeMap
                *  - mvp, maxSteps, aabbMin, aabbMax, renderHeight
                *  - lightAmount, mode, binarySearch, LightData
                * 3. Render using glDrawElements
                * 4. Unbind the VAO.
                */
            }
        }

        else {
            // Draw Flat Image
            if (showFlatQuad) {
                /*
                * == Code Snippet 11 ==
                * 1. Bind the Quad Shader and VAO.
                * 2. Setup all the shader uniform parameters:
                *  - inputTexture, mvp, height
                * 3. Render using glDrawElements
                * 4. Unbind the VAO.
                */
            }


            // Draw dots
            if (showDots) {

                /*
                * == Code Snippet 12 ==
                * 1. Bind the Dot Shader and VAO.
                * 2. Setup all the shader uniform parameters:
                *  - normalMap, colorMap, edgeMap
                *  - mvp, mode, renderHeight, renderSize
                *  - minDistanceToCamera, maxDistanceToCamera, cameraPos
                * 3. Set the GL Point size
                * 4. Render using glDrawArrays
                * 5. Unbind the VAO.
                */


                // Render wireframe
                if (showWireframe) {
                    /*
                    * == Code Snippet 13 ==
                    * 1. Bind the wireframe VAO.
                    * 4. Render using glDrawElements
                    * 5. Unbind the VAO.
                    */
                }

                // Render lines
                if (showLines) {
                    /*
                    * == Code Snippet 14 ==
                    * 1. Bind the Line Shader and Dot VAO.
                    * 2. Setup all the shader uniform parameters:
                    *  - normalMap, colorMap, edgeMap
                    *  - mvp, mode, renderHeight, renderSize
                    *  - minDistanceToCamera, maxDistanceToCamera, cameraPos
                    * 4. Render using glDrawArrays
                    * 5. Unbind the VAO.
                    */
                }
            }

            // Draw triangles
            if (showTriangles) {
                /*
                * == Code Snippet 15 ==
                * 1. Bind the Triangle Shader and VAO.
                * 2. Setup all the shader uniform parameters:
                *  - normalMap, colorMap, edgeMap
                *  - mvp, mode, renderHeight, renderSize
                *  - lightAmount, LightData
                * 4. Render using glDrawElements
                * 5. Unbind the VAO.
                */
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
    glDeleteBuffers(1, &colormapVBO);
    glDeleteBuffers(1, &quadEBO);
    glDeleteBuffers(1, &wireframeEBO);
    glDeleteBuffers(1, &triangleEBO);
    glDeleteBuffers(1, &raytracingEBO);
    glDeleteBuffers(1, &colormapEBO);
    glDeleteVertexArrays(1, &lightVAO);
    glDeleteVertexArrays(1, &dotVAO);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteVertexArrays(1, &wireframeVAO);
    glDeleteVertexArrays(1, &triangleVAO);
    glDeleteVertexArrays(1, &raytracingVAO);
    glDeleteVertexArrays(1, &colormapVAO);
    glDeleteBuffers(1, &lightUBO);

    return 0;
}

