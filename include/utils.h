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
    Slope = 2,
    Edges = 3
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
    int width = 0;                     // The image width  (in pixels)
    int height = 0;                    // The image height (in pixels)
    float renderSize = 1.0f;           // The dot cloud render width  (from the TOML config)
    float renderHeight = 1.0f;         // The dot cloud render height (from the TOML config)
};

// Light
struct Light {
    glm::vec3 position;
    glm::vec3 color;
};

//===========================================================================


// GLOBAL VARIABLES
const int WIDTH = 1200;             // Program window width          
const int HEIGHT = 800;             // Program window height     
const int MAX_LIGHT_AMOUNT = 20;    // Max amount of light sources in the scene
ImageData imageData;                // image data for the 3D visualization
std::vector<ImageData> colorMaps;   // list of color maps images
int activeColorMap = 0;             // index of the color map currently in use
Window* WINDOW;                     // Pointer to the GL window
Trackball* TRACKBALL;               // Pointer to the camera trackball



//=========================Config Loading Functions==========================

/**
 * @brief Parses a TOML array into a 3D vector.
 * 
 * This function parses an array in a TOML file into a 3-components float vector.
 * 
 * @param array TOML array parsed from the configuration file.
 * @return A 3-components float vector.
 */
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


/**
 * @brief Reads an image from a file and returns a list of Pixels.
 * 
 * This function reads an image file and converts it to an array of Pixels values.
 * It detects if the image is 8bits or 16bits, and converts 8bits images into 16bits Pixel values.
 * 
 * @param filePath Path to the image file.
 * @param width Reference to an integer where the width of the loaded image will be stored.
 * @param height Reference to an integer where the height of the loaded image will be stored.
 * @return An array of Pixel values.
 */
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


/**
 * @brief Configures the default scene based on the TOML config file.
 * 
 * This function parses the config file.
 * It loads the heightmap and the colormap images.
 * It then configures the camera trackball and the lights in the default scene.
 * 
 * @param trackball Reference to the camera trackball object to configure.
 * @param image Reference to an ImageData where the heightmap will be stored.
 * @param colorMaps Reference to an array of ImageData where the colormaps will be stored.
 * @param lightList Refernece to an array of Light where the default scene lights will be stored.
 */
static void readInitialConfig(Trackball* trackball, ImageData& image, std::vector<ImageData>& colorMaps, std::vector<Light>& lightList) {
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
    lightList = std::vector<Light>{};
    size_t lightAmount = config["lights"]["positions"].as_array()->size();
    for (size_t i = 0; i < lightAmount; ++i) {
        auto pos = tomlArrayToVec3(config["lights"]["positions"][i].as_array());
        auto color = tomlArrayToVec3(config["lights"]["colors"][i].as_array());
        lightList.emplace_back(Light{ pos, color });
    }

    // read camera settings from TOML and setup trackball
    glm::vec3 lookAt = tomlArrayToVec3(config["camera"]["look_at"].as_array());
    glm::vec3 rotations = tomlArrayToVec3(config["camera"]["rotations"].as_array());
    float fovY = config["camera"]["fovy"].value_or(50.0f);
    float dist = config["camera"]["dist"].value_or(1.0f);
    trackball->setCamera(lookAt, rotations, dist);

    // read image path from TOML
    std::cout << "Loading image... ";
    float renderSize = config["data"]["render_size"].value_or(10.0f);
    float renderHeight = config["data"]["render_height"].value_or(5.0f);
    auto dataPath = std::string(RESOURCE_ROOT) + config["data"]["path"].value_or("resources/Terrains/default.png");
    int width, height;
    const std::vector<Pixel> pixels = loadPixelsFromImage(dataPath.c_str(), width, height);
    std::cout << "done." << std::endl;
    if (width != height) {
        std::cout << "ERROR: image must have be a square!" << std::endl << "  => Basic Image is loaded instead." << std::endl;
        const std::vector<Pixel> basicPixels = { Pixel(0, 0, 0), Pixel(0, 0, 0), Pixel(65365, 65365, 65365), Pixel(65365, 65365, 65365) };
        image = { basicPixels, 2, 2, renderSize, renderHeight };
    }
    else {
        std::cout << "Loaded image " << dataPath.c_str() << " with dimensions " << width << "x" << height << std::endl;
        image = { pixels, width, height, renderSize, renderHeight };
    }

    colorMaps = std::vector<ImageData>{};
    size_t colorMapsAmount = config["gradient"]["paths"].as_array()->size();
    for (size_t i = 0; i < colorMapsAmount; ++i) {
        auto path = std::string(RESOURCE_ROOT) + config["gradient"]["paths"][i].value_or("resources/Colormaps/viridis.png");
        const std::vector<Pixel> color_map_pixels = loadPixelsFromImage(path.c_str(), width, height);
        std::cout << "Loaded color map " << path.c_str() << " with dimensions " << width << "x" << height << std::endl;
        colorMaps.emplace_back(ImageData{ color_map_pixels, width, height, 0.0, 0.0 });
    }
}

//===========================================================================

