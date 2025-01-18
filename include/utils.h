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
#include <iostream>
#include <numeric>
#include <optional>
#include <span>
#include <toml/toml.hpp>
#include <vector>
#include <array>
#include <cstring>


//============================ENUMS AND STRUCTS===============================

// Dot Rendering Mode Enum
enum class RenderingMode {
    FixedColor = 0,
    Gradient = 1,
    Slope = 2
};

// Pixels and Image Data
struct Pixel {
    unsigned short R = 0;
    unsigned short G = 0;
    unsigned short B = 0;
};

// Image Data
struct ImageData {
    std::vector<Pixel> pixels;         // The image pixel data
    int width = 0;                     // The image width (in pixels)
    int height = 0;                    // The image height (in pixels)
    float render_size = 1.0f;          // The dot cloud render width
    float render_height = 1.0f;        // The dot cloud render height
};

// Light
struct Light {
    glm::vec3 position;
    glm::vec3 color;
};

//===========================================================================


// GLOBAL VARIABLES
const int WIDTH = 1200;
const int HEIGHT = 800;
const int MAX_LIGHT_AMOUNT = 20;
ImageData image_data;
Window* WINDOW;
Trackball* TRACKBALL;



//=========================Config Loading Functions==========================

static glm::vec3 tomlArrayToVec3(const toml::array* array)
{
    glm::vec3 output{ 0.0f };

    if (array) {
        int i = 0;
        array->for_each([&](auto&& elem) {
            if (elem.is_number()) {
                if (i > 2)
                    return;
                output[i] = static_cast<float>(elem.as_floating_point()->get());
                i += 1;
            }
            else {
                std::cerr << "Error: Expected a number in array, got " << elem.type() << std::endl;
                return;
            }
            });
    }

    return output;
}


static std::vector<Pixel> loadPixelsFromImage(const char* filePath, int& width, int& height) {
    // Reserve memory for the pixel data.
    std::vector<Pixel> pixels;
    pixels.reserve(width * height);

    // Try loading the image in 16-bits RGB.
    int channels;
    unsigned short* data16 = stbi_load_16(filePath, &width, &height, &channels, STBI_rgb);

    // If the image is correctly loaded, store the 16-bits data in the pixel array.
    if (data16) {
        for (int i = 0; i < width * height; ++i) {
            Pixel pixel{ 
                data16[i * 3 + 0], // R
                data16[i * 3 + 1], // G
                data16[i * 3 + 2]  // B
            };
            pixels.push_back(pixel);
        }
        stbi_image_free(data16);
        return pixels;
    }

    // Try loading the image in 8-bits RGB.
    unsigned char* data8 = stbi_load(filePath, &width, &height, &channels, STBI_rgb);
    // If loading failed, print error and return.
    if (!data8) {
        std::cerr << "Failed to load image: " << filePath << std::endl;
        return {};
    }
    
    // If the image is correctly loaded, store the 8-bits data in the pixel array.
    for (int i = 0; i < width * height; ++i) {
        // Scale 8-bit to 16-bit
        Pixel pixel{
            static_cast<uint16_t>(data8[i * 3 + 0] * 257), // R
            static_cast<uint16_t>(data8[i * 3 + 1] * 257), // G
            static_cast<uint16_t>(data8[i * 3 + 2] * 257)  // B
        };
        pixels.push_back(pixel);
    }
    stbi_image_free(data8);
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
        lights_list.emplace_back(Light{ pos, color });
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
    if (image_width != image_height) {
        std::cout << "ERROR: image must have be a square!" << std::endl << "  => Basic Image is loaded instead." << std::endl;
        const std::vector<Pixel> basicPixels = { Pixel(0, 0, 0), Pixel(0, 0, 0), Pixel(65365, 65365, 65365), Pixel(65365, 65365, 65365) };
        image = { basicPixels, 2, 2, render_size, render_height };
    }
    else {
        std::cout << "Loaded image " << data_path.c_str() << " with dimensions " << image_width << "x" << image_height << std::endl;
        image = { pixels, image_width, image_height, render_size, render_height };
    }

    return trackball;
}

//===========================================================================