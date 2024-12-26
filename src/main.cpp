// Disable compiler warnings in third-party code (which we cannot change).
#include <framework/disable_all_warnings.h>
#include <framework/opengl_includes.h>
DISABLE_WARNINGS_PUSH()
// Include glad before glfw3
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <stb/stb_image.h>
DISABLE_WARNINGS_POP()
#include <algorithm>
#include <cassert>
#include <cstdlib> // EXIT_FAILURE
#include <framework/shader.h>
#include <framework/trackball.h>
#include <framework/window.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl2.h>
#include <iostream>
#include <numeric>
#include <optional>
#include <span>
#include <toml/toml.hpp>
#include <vector>
#include <array>

//============================ENUMS AND STRUCTS===============================

// Dot Rendering Mode Enum
enum class DotRenderingMode {
    FixedColor = 0,
    HeightGradient = 1,
    CameraDistanceGradient = 2
};

// Pixels and Image Data
struct Pixel {
    unsigned char R;
    unsigned char G;
    unsigned char B;
};

// Image Data
struct ImageData {
    std::vector<Pixel> pixels;  // The image pixel data
    int width;                  // The image width (in pixels)
    int height;                 // The image height (in pixels)
    float render_size;          // The dot cloud render width
    float render_height;        // The dot cloud render height
};

// Light
struct Light {
    glm::vec3 position;
    glm::vec3 color;
};

//===========================================================================



//==============================Configuration================================

const int WIDTH = 1200;
const int HEIGHT = 800;
ImageData image_data;


bool show_imgui = true;

// Quad
bool render_quad = false;

// Dots
bool render_dots = false;
bool render_lines = false;
bool render_wireframe = false;
float dot_size = 2.0f;
DotRenderingMode dot_render_mode = DotRenderingMode::FixedColor;
glm::vec3 dot_color_1 {1.0f, 0.0f, 0.0f};
glm::vec3 dot_color_2 { 0.0f, 1.0f, 0.0f };

// Lights
std::vector<Light> lights {};
size_t selectedLightIndex = 0;

//===========================================================================



//============================UI Helper Functions============================

static void printHelp()
{
    Trackball::printHelp();
    std::cout << std::endl;
    std::cout << "Program Usage:" << std::endl;
    std::cout << "=============================" << std::endl;
    std::cout << "TODO: Print Message + Keyboard Shortcuts" << std::endl;
    std::cout << "=============================" << std::endl;
}

static void resetLights()
{
    lights.clear();
    lights.push_back(Light { glm::vec3(0, 0, 3), glm::vec3(1) });
    selectedLightIndex = 0;
}

static void selectNextLight()
{
    selectedLightIndex = (selectedLightIndex + 1) % lights.size();
}

static void selectPreviousLight()
{
    if (selectedLightIndex == 0)
        selectedLightIndex = lights.size() - 1;
    else
        --selectedLightIndex;
}

static void renderGUI()
{
    // UI Menu
    if (!show_imgui)
        return;

    // Title
    ImGui::Begin("3D Data Visualizer");
    ImGui::Text("Press \\ to show/hide this menu");
    ImGui::Separator();
    
    // Quads Rendering
    ImGui::Text("Simple Quad");
    ImGui::Checkbox("Render Flat Image", &render_quad);
    ImGui::Separator();

    // Dots Rendering
    ImGui::Text("Dots");
    ImGui::Checkbox("Show Dots", &render_dots);
    ImGui::InputFloat("Dot Size", &dot_size);
    ImGui::ColorEdit3("Color 1", &dot_color_1[0]);
    ImGui::ColorEdit3("Color 2", &dot_color_2[0]);
    // Dropdown for dot render style
    std::array dot_render_mode_names{ "Fixed Color", "Gradient based on Height", "Gradient based on distance to Camera" };
    int current_dot_render_mode = static_cast<int>(dot_render_mode);
    ImGui::Combo("Render Mode", &current_dot_render_mode, dot_render_mode_names.data(), (int)dot_render_mode_names.size());
    dot_render_mode = static_cast<DotRenderingMode>(current_dot_render_mode);
    ImGui::Checkbox("Show Lines", &render_lines);
    ImGui::Checkbox("Show Wireframe", &render_wireframe);
    ImGui::Separator();

    //Lights Rendering
    ImGui::Text("Lights");
    std::vector<std::string> itemStrings = {};
    for (size_t i = 0; i < lights.size(); i++) {
        auto string = "Light " + std::to_string(i);
        itemStrings.push_back(string);
    }
    std::vector<const char*> itemCStrings = {};
    for (const auto& string : itemStrings) {
        itemCStrings.push_back(string.c_str());
    }
    int tempSelectedItem = static_cast<int>(selectedLightIndex);
    if (ImGui::ListBox("Lights", &tempSelectedItem, itemCStrings.data(), (int) itemCStrings.size(), 4)) {
        selectedLightIndex = static_cast<size_t>(tempSelectedItem);
    }
    if (ImGui::Button("Reset Lights")) {
        resetLights();
    }

    // End GUI
    ImGui::End();
    ImGui::Render();
}

//===========================================================================



//=========================Config Loading Functions==========================

static glm::vec3 tomlArrayToVec3(const toml::array* array)
{
    glm::vec3 output {0.0f};

    if (array) {
        int i = 0;
        array->for_each([&](auto&& elem) {
            if (elem.is_number()) {
                if (i > 2)
                    return;
                output[i] = static_cast<float>(elem.as_floating_point()->get());
                i += 1;
            } else {
                std::cerr << "Error: Expected a number in array, got " << elem.type() << std::endl;
                return;
            }
        });
    }

    return output;
}

static std::vector<Pixel> loadPixelsFromImage(const char* filePath, int& width, int& height) {
    // Load image data
    int channels;
    unsigned char* data = stbi_load(filePath, &width, &height, &channels, STBI_rgb);
    if (!data) {
        std::cerr << "Failed to load image: " << filePath << std::endl;
        return {};
    }

    // Extract pixels from image data
    std::vector<Pixel> pixels;
    pixels.reserve(width * height);
    for (int i = 0; i < width * height; ++i) {
        Pixel pixel{ data[i * 3 + 0], data[i * 3 + 1], data[i * 3 + 2] }; // RGB values
        pixels.push_back(pixel);
    }

    // Free the image data
    stbi_image_free(data);
    return pixels;
}

static Trackball readInitialConfig(Window* window, ImageData& image, std::vector<Light>& lights_list) {
    const GLubyte* version = glGetString(GL_VERSION);
    std::cout << "OpenGL Version: " << version << std::endl;

    // Parse initial scene config TOML
    std::cout << "Loading TOML config... ";
    toml::table config;
    try {
        config = toml::parse_file(RESOURCE_ROOT "resources/default_scene.toml");
        std::cout << "done." << std::endl;
    }
    catch (const toml::parse_error&) {
        std::cerr << "parsing failed" << std::endl;
    }

    // read lights from TOML
    lights_list = std::vector<Light>{};
    size_t num_lights = config["lights"]["positions"].as_array()->size();
    for (size_t i = 0; i < num_lights; ++i) {
        auto pos = tomlArrayToVec3(config["lights"]["positions"][i].as_array());
        auto color = tomlArrayToVec3(config["lights"]["colors"][i].as_array());
        lights.emplace_back(Light{ pos, color });
    }

    // read camera settings from TOML and setup trackball
    glm::vec3 look_at = tomlArrayToVec3(config["camera"]["lookAt"].as_array());
    glm::vec3 rotations = tomlArrayToVec3(config["camera"]["rotations"].as_array());
    float fovY = config["camera"]["fovy"].value_or(50.0f);
    float dist = config["camera"]["dist"].value_or(1.0f);
    Trackball trackball{ window, glm::radians(fovY) };
    trackball.setCamera(look_at, rotations, dist);

    // read image path from TOML
    std::cout << "Loading image... ";
    float render_size = config["data"]["render_size"].value_or(10.0f);
    float render_height = config["data"]["render_height"].value_or(5.0f);
    auto data_path = std::string(RESOURCE_ROOT) + config["data"]["path"].value_or("resources/default.png");
    int image_width, image_height;
    const std::vector<Pixel> pixels = loadPixelsFromImage(data_path.c_str(), image_width, image_height);
    std::cout << "done." << std::endl;
    std::cout << "Loaded image " << data_path.c_str() << " with dimensions " << image_width << "x" << image_height << std::endl;
    image = { pixels, image_width, image_height, render_size, render_height };

    return trackball;
}

//===========================================================================

static float heightFromPixel(Pixel pixel) {
    return 0.299f * static_cast<float>(pixel.R) / 255.0f
         + 0.587f * static_cast<float>(pixel.G) / 255.0f
         + 0.114f * static_cast<float>(pixel.B) / 255.0f;
}

static int loadDotVertices(ImageData data, bool lines, bool wireframe, GLuint* dotVAO, GLuint* dotVBO, float* maxHeight, float* minHeight) {
    std::vector<glm::vec3> dotVertices, resultVertices;
    dotVertices.reserve(data.width * data.height);
    *minHeight = data.render_size;
    *maxHeight = 0.0f;

    // Find dot vertices
    for (int z = 0; z < data.height; ++z) {
        for (int x = 0; x < data.width; ++x) {
            const Pixel& pixel = data.pixels[z * data.width + x];
            float y = heightFromPixel(pixel) * data.render_height;
            if (y < *minHeight) *minHeight = y;
            else if (y > *maxHeight) *maxHeight = y;

            float xPos = data.render_size * (static_cast<float>(x) / static_cast<float>(data.width) - 0.5f);
            float zPos = data.render_size * (static_cast<float>(z) / static_cast<float>(data.height) - 0.5f);
            dotVertices.emplace_back(glm::vec3(xPos, y, zPos));
                
            // Add another vertex to draw a vertical line
            if (lines) {
                dotVertices.emplace_back(glm::vec3(xPos, 0.0f, zPos));
            }
        }
    }

    // Find wireframe vertices
    if (wireframe) {
        resultVertices.reserve(data.width*data.height*4);
        for (int z = 0; z < data.height; ++z) {
            for (int x = 0; x < data.width; ++x) {
                glm::vec3 current = dotVertices[z * data.height + x];

                if (z + 1 < data.height) {
                    glm::vec3 neighbor = dotVertices[(z+1) * data.height + x];
                    resultVertices.push_back(current);
                    resultVertices.push_back(neighbor);
                }

                if (x + 1 < data.width) {
                    glm::vec3 neighbor = dotVertices[z * data.height + x + 1];
                    resultVertices.push_back(current);
                    resultVertices.push_back(neighbor);
                }
            }
        }
    }
    else {
        resultVertices = dotVertices;
    }

    // Clean previous VAO and VBO
    glDeleteVertexArrays(1, dotVAO);
    glDeleteBuffers(1, dotVBO);
    
    // Generate new VAO and VBO
    glGenVertexArrays(1, dotVAO);
    glGenBuffers(1, dotVBO);
    glBindVertexArray(*dotVAO);

    // Upload all the dot positions to the VBO
    glBindBuffer(GL_ARRAY_BUFFER, *dotVBO);
    glBufferData(GL_ARRAY_BUFFER, resultVertices.size() * sizeof(glm::vec3), resultVertices.data(), GL_STATIC_DRAW);

    // Define the vertex attribute for position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return resultVertices.size();
}


// Program entry point. Everything starts here.
int main(int argc, char** argv)
{
    // Create program window
    Window window{ "3D Data Visualizer", glm::ivec2(WIDTH, HEIGHT), OpenGLVersion::GL41 };

    // Parse initial scene config TOML
    Trackball trackball = readInitialConfig(&window, image_data, lights);

    // Create dot cloud + lines vertices + wireframe vertices
    GLuint dotVAO, dotVBO, lineVAO, lineVBO, wireframeVAO, wireframeVBO;
    float minHeight, maxHeight;
    int dotAmount = loadDotVertices(image_data, false, false, &dotVAO, &dotVBO, &maxHeight, &minHeight);
    loadDotVertices(image_data, true, false, &lineVAO, &lineVBO, &maxHeight, &minHeight);
    int wireframeDotAmount = loadDotVertices(image_data, false, true, &wireframeVAO, &wireframeVBO, &maxHeight, &minHeight);

    // Light VAO and VBO
    GLuint lightVAO, lightVBO;
    glGenVertexArrays(1, &lightVAO);
    glGenBuffers(1, &lightVBO);
    glBindVertexArray(lightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lightVBO);

    const Shader lightShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/light_vertex.glsl")
                                              .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/light_frag.glsl")
                                              .build();
    const Shader dotShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/dot_vertex.glsl")
                                            .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/dot_frag.glsl")
                                            .build();


    // Main loop.
    while (!window.shouldClose()) {
        // Update input and UI
        window.updateInput();
        renderGUI();
        
        // Clear the framebuffer to black and depth to maximum value (ranges from [-1.0 to +1.0]).
        glViewport(0, 0, window.getWindowSize().x, window.getWindowSize().y);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Compute distance to camera
        const glm::vec3 cameraPos = trackball.position();
        const float maxSize = std::sqrtf(image_data.render_height*image_data.render_height + 
                                        image_data.render_size*image_data.render_size/2.0f);
        float minDistanceToCamera = glm::length(cameraPos) - maxSize;
        float maxDistanceToCamera = glm::length(cameraPos) + maxSize;
        if (minDistanceToCamera < 0.0f) {
            minDistanceToCamera = 0.0f;
        }

        // Set model/view/projection matrix.
        const glm::mat4 model { 1.0f };
        const glm::mat4 view = trackball.viewMatrix();
        const glm::mat4 projection = trackball.projectionMatrix();
        const glm::mat4 mvp = projection * view * model;
        
        // Draw dots
        if (render_dots) {

            //Shader and variable setup
            dotShader.bind();
            const int mode = static_cast<int>(dot_render_mode);
            glUniform3fv(dotShader.getUniformLocation("color1"), 1, glm::value_ptr(dot_color_1));
            glUniform3fv(dotShader.getUniformLocation("color2"), 1, glm::value_ptr(dot_color_2));
            glUniform1iv(dotShader.getUniformLocation("mode"), 1, &mode);
            glUniform1f(dotShader.getUniformLocation("minHeight"), minHeight);
            glUniform1f(dotShader.getUniformLocation("maxHeight"), maxHeight);
            glUniform1f(dotShader.getUniformLocation("minDistanceToCamera"), minDistanceToCamera);
            glUniform1f(dotShader.getUniformLocation("maxDistanceToCamera"), maxDistanceToCamera);
            glUniformMatrix4fv(dotShader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
            glUniform3fv(dotShader.getUniformLocation("cameraPos"), 1, glm::value_ptr(cameraPos));
            
            // Render dots
            glPointSize(dot_size);
            glBindVertexArray(dotVAO);
            glDrawArrays(GL_POINTS, 0, dotAmount);
            
            // Render lines
            if (render_lines) {
                glBindVertexArray(lineVAO);
                glDrawArrays(GL_LINES, 0, dotAmount*2);
            }

            // Render wireframe
            if (render_wireframe) {
                glBindVertexArray(wireframeVAO);
                glDrawArrays(GL_LINES, 0, wireframeDotAmount);
            }
            glBindVertexArray(0);
        }

        // Draw lights as (square) points.
        lightShader.bind();
        {
            const glm::vec4 screenPos = mvp * glm::vec4(lights[selectedLightIndex].position, 1.0f);
            const glm::vec3 color { 1, 1, 0 };

            glPointSize(15.0f);
            glUniform4fv(lightShader.getUniformLocation("pos"), 1, glm::value_ptr(screenPos));
            glUniform3fv(lightShader.getUniformLocation("color"), 1, glm::value_ptr(color));
            glBindVertexArray(lightVAO);
            glDrawArrays(GL_POINTS, 0, 1);
            glBindVertexArray(0);
        }
        for (const Light& light : lights) {
            const glm::vec4 screenPos = mvp * glm::vec4(light.position, 1.0f);

            glPointSize(10.0f);
            glUniform4fv(lightShader.getUniformLocation("pos"), 1, glm::value_ptr(screenPos));
            glUniform3fv(lightShader.getUniformLocation("color"), 1, glm::value_ptr(light.color));
            glBindVertexArray(lightVAO);
            glDrawArrays(GL_POINTS, 0, 1);
            glBindVertexArray(0);
        }

        // Present result to the screen.
        window.swapBuffers();
    }

    // Cleanup
    glDeleteBuffers(1, &lightVBO);
    glDeleteBuffers(1, &dotVBO);
    glDeleteVertexArrays(1, &lightVAO);
    glDeleteVertexArrays(1, &dotVAO);

    return 0;
}

static std::optional<glm::vec3> getWorldPositionOfPixel(const Trackball& trackball, const glm::vec2& pixel)
{
    float depth;
    glReadPixels(static_cast<int>(pixel.x), static_cast<int>(pixel.y), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

    if (depth == 1.0f) {
        // This is a work around for a bug in GCC:
        // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80635
        //
        // This bug will emit a warning about a maybe uninitialized value when writing:
        // return {};
        constexpr std::optional<glm::vec3> tmp;
        return tmp;
    }

    // Coordinates convert from pixel space to OpenGL screen space (range from -1 to +1)
    const glm::vec3 win { pixel, depth };

    // View matrix
    const glm::mat4 view = trackball.viewMatrix();
    const glm::mat4 projection = trackball.projectionMatrix();

    const glm::vec4 viewport { 0, 0, WIDTH, HEIGHT };
    return glm::unProject(win, view, projection, viewport);
}

