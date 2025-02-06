#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl2.h>
#include <glutils.h>

//==============================Configuration================================

// General
bool showGui = true;                // Show the menu
bool showRaytracingTab = false;     // Wether the rasterization or ray tracing tab is active
float renderHeight = 5.0f;          // Current data render height
float renderSize = 20.0f;           // Current data render width
int edgeSmallBlurSize = 10;         // Small gaussian radius for edge detection
int edgeLargeBlurSize = 100;        // Large gaussian radius for edge detection
bool showColorMap = true;           // Show the active color map in the corner of the screen

// Quad
bool showFlatQuad = false;          // Show the image as a flat plane
float height = 0.0f;                // Height at which the flat plane is rendered

// Dots
bool showDots = false;              // Show the dot cloud
bool showLines = false;             // Show the vertical lines
bool showWireframe = false;         // Show the wireframe
float dotSize = 2.0f;               // Dot render size
float maxRenderDistance = 20.0f;    // Maximum distance at which dots should be visible
RenderingMode renderMode = RenderingMode::FixedColor;   // Render mode

// Triangles
bool showTriangles = false;         // Show triangles between vertices

// Lights
std::vector<Light> lights{};        // List of light sources
size_t selectedLightIndex = 0;      // Index of the currently selected light

// Ray Tracing
bool showRaytracing = false;        // Show the data using ray tracing
int maxSteps = 100;                 // Amount of ray tracing steps
bool useBinarySearch = false;       // Whether binary search is used to get a cleaner result

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
    std::cout << "C       -> apply the next color map" << std::endl;
    std::cout << "L       -> place the light source at the current camera position" << std::endl;
    std::cout << "Shift+L -> add an additional light source at the current camera position" << std::endl;
    std::cout << "Down    -> select the next light source" << std::endl;
    std::cout << "Up      -> select the previous light source" << std::endl;
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

// Draws the UI menu
static void renderGUI()
{
    // UI Menu
    if (!showGui)
        return;

    // Title
    ImGui::Begin("3D Data Visualizer");
    ImGui::Text("Press TAB to show/hide this menu");
    ImGui::Separator();

    // Base Parameters
    ImGui::Text("Base Parameters");
    ImGui::Checkbox("Show Color Map", &showColorMap);
    ImGui::InputFloat("Width", &renderSize);
    ImGui::InputFloat("Height", &renderHeight);

    // Dropdown for render mode
    std::array renderMode_names{ "Fixed Color", "Height Gradient", "Slope Gradient", "Valley And Peak Indicator"};
    int current_renderMode = static_cast<int>(renderMode);
    ImGui::Combo("Render Mode", &current_renderMode, renderMode_names.data(), (int)renderMode_names.size());
    renderMode = static_cast<RenderingMode>(current_renderMode);

    if (renderMode == RenderingMode::Edges) {
        ImGui::InputInt("Large Gaussian Radius", &edgeLargeBlurSize);
        ImGui::InputInt("Small Gaussian Radius", &edgeSmallBlurSize);
    }
    ImGui::Separator();

    // Tabs
    if (ImGui::BeginTabBar("Mode")) {
        // Rasterization Tab
        if (ImGui::BeginTabItem("Rasterization")) {
            showRaytracingTab = false;

            // Quad Rendering
            ImGui::Checkbox("Render Flat Image", &showFlatQuad);
            if (showFlatQuad) {
                ImGui::SliderFloat("Height", &height, -renderHeight, renderHeight);
            }
            ImGui::Separator();

            // Dots Rendering
            ImGui::Text("Dots");
            ImGui::Checkbox("Show Dots", &showDots);
            if (showDots) {
                ImGui::Indent(0.0f);
                ImGui::InputFloat("Dot Size", &dotSize);
                ImGui::InputFloat("Max Render Distance", &maxRenderDistance);
                ImGui::Checkbox("Show Lines", &showLines);
                ImGui::Checkbox("Show Wireframe", &showWireframe);
                ImGui::Unindent();
            }
            ImGui::Separator();


            // Triangles Rendering
            ImGui::Text("Triangles");
            ImGui::Checkbox("Show Triangles", &showTriangles);
            ImGui::Separator();

            ImGui::EndTabItem();
        }


        // RayTracing Tab
        if (ImGui::BeginTabItem("Ray Tracing")) {
            showRaytracingTab = true;
            ImGui::Checkbox("Ray Tracing", &showRaytracing);
            if (showRaytracing) {
                ImGui::DragInt("Steps", &maxSteps, 0.5f, 0, 1000);
                ImGui::Checkbox("Binary Search", &useBinarySearch);
            }
            ImGui::Separator();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

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

//===========================================================================