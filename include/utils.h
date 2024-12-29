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


//============================ENUMS AND STRUCTS===============================

// Dot Rendering Mode Enum
enum class DotRenderingMode {
    FixedColor = 0,
    HeightGradient = 1,
    CameraDistanceGradient = 2
};

// Pixels and Image Data
struct Pixel {
    unsigned char R = 0;
    unsigned char G = 0;
    unsigned char B = 0;
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
ImageData image_data;
Window* WINDOW;
Trackball* TRACKBALL;