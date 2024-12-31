#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl2.h>
#include <utils.h>

//==============================Configuration================================

// General
bool show_imgui = true;
glm::vec3 color_1{ 1.0f, 0.0f, 0.0f };
glm::vec3 color_2{ 0.0f, 1.0f, 0.0f };
float render_height = 5.0f;
float render_size = 20.0f;

// Quad
bool render_quad = false;

// Dots
bool render_dots = false;
bool render_lines = false;
bool render_wireframe = false;
float dot_size = 2.0f;
float max_render_distance = 20.0f;
RenderingMode render_mode = RenderingMode::FixedColor;

// Triangles
bool render_triangles = false;

// Lights
std::vector<Light> lights{};
size_t selectedLightIndex = 0;

//===========================================================================



//============================UI Helper Functions============================

static void printHelp()
{
    std::cout << std::endl << "********************Camera Usage:********************" << std::endl << std::endl;
    Trackball::printHelp();
    std::cout << std::endl;
    std::cout << "*****************Keyboard Shortcuts:*****************" << std::endl << std::endl;
    std::cout << "TAB -> show/hide menu" << std::endl;
    std::cout << "H   -> show help" << std::endl;
    std::cout << "______________________" << std::endl << std::endl;
    std::cout << "L       -> place the light source at the current camera position" << std::endl;
    std::cout << "Shift+L -> add an additional light source at the current camera position" << std::endl;
    std::cout << "+       -> choose next light source" << std::endl;
    std::cout << "-       -> choose previous light source" << std::endl;
    std::cout << "DEL     -> delete selected light source" << std::endl;
    std::cout << "N       -> clear all light sources and reinitialize with one" << std::endl;
    std::cout << "______________________" << std::endl << std::endl;
    std::cout << "R       -> add 0.1 to the red channel of the selected light" << std::endl;
    std::cout << "G       -> add 0.1 to the green channel of the selected light" << std::endl;
    std::cout << "B       -> add 0.1 to the blue channel of the selected light" << std::endl;
    std::cout << "Shift+R -> substract 0.1 from the red channel of the selected light" << std::endl;
    std::cout << "Shift+G -> substract 0.1 from the green channel of the selected light" << std::endl;
    std::cout << "Shift+B -> substract 0.1 from the blue channel of the selected light" << std::endl;
    std::cout << std::endl << "*****************************************************" << std::endl << std::endl;
}

static void resetLights()
{
    lights.clear();
    lights.push_back(Light{ glm::vec3(0, 0, 3), glm::vec3(1) });
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

static void deleteLight()
{
    if (lights.size() <= 1)
        return;

    lights.erase(lights.begin() + selectedLightIndex);

    if (selectedLightIndex > 0)
        selectedLightIndex -= 1;
}

// Prints the time elapsed during the start and end points
static void printElapsedTime(std::chrono::time_point<std::chrono::high_resolution_clock> start,
    std::chrono::time_point<std::chrono::high_resolution_clock> end,
    const char* text) {
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    if (duration_ms < 2000) {
        std::cout << text << " done in " << duration_ms << " ms." << std::endl;
    }
    else {
        auto duration_s = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
        std::cout << text << " done in " << duration_s << " s." << std::endl;
    }
}

//===========================================================================


// Key Pressed Handler
static void keyPressedHandler(int key, int /* scancode */, int action, int /* mods */) {
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        show_imgui = !show_imgui;
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
    case GLFW_KEY_MINUS: {
        selectPreviousLight();
        return;
    }
    case GLFW_KEY_EQUAL: {
        if (shiftPressed) // '+' pressed (unless you use a weird keyboard layout).
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

// Draws the UI menu
static void renderGUI()
{
    // UI Menu
    if (!show_imgui)
        return;

    // Title
    ImGui::Begin("3D Data Visualizer");
    ImGui::Text("Press TAB to show/hide this menu");
    ImGui::Separator();

    // Quads Rendering
    ImGui::Text("Base Parameters");
    ImGui::Checkbox("Render Flat Image", &render_quad);
    ImGui::ColorEdit3("Color 1", &color_1[0]);
    ImGui::ColorEdit3("Color 2", &color_2[0]);
    ImGui::InputFloat("Width", &render_size);
    ImGui::InputFloat("Height", &render_height);
    // Dropdown for render mode
    std::array render_mode_names{ "Fixed Color", "Height Gradient" };
    int current_render_mode = static_cast<int>(render_mode);
    ImGui::Combo("Render Mode", &current_render_mode, render_mode_names.data(), (int)render_mode_names.size());
    render_mode = static_cast<RenderingMode>(current_render_mode);
    ImGui::Separator();

    // Dots Rendering
    ImGui::Text("Dots");
    ImGui::Checkbox("Show Dots", &render_dots);
    if (render_dots) {
        ImGui::Indent(0.0f);
        ImGui::InputFloat("Dot Size", &dot_size);
        ImGui::InputFloat("Max Render Distance", &max_render_distance);
        ImGui::Checkbox("Show Lines", &render_lines);
        ImGui::Checkbox("Show Wireframe", &render_wireframe);
        ImGui::Unindent();
    }
    ImGui::Separator();


    // Triangles Rendering
    ImGui::Text("Triangles");
    ImGui::Checkbox("Show Triangles", &render_triangles);
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
    if (ImGui::ListBox("Lights", &tempSelectedItem, itemCStrings.data(), (int)itemCStrings.size(), 4)) {
        selectedLightIndex = static_cast<size_t>(tempSelectedItem);
    }
    if (ImGui::Button("Reset Lights")) {
        resetLights();
    }

    // End GUI
    ImGui::End();
    ImGui::Render();
}