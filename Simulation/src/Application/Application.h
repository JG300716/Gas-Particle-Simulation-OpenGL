#pragma once

#include "../Render/Renderer.h"
#include "../Render/Camera.h"
#include "../Simulation/SmokeSimulation.h"
#include "../Simulation/ImGuiControls.h"
#include <GLFW/glfw3.h>

class Application {
public:
    Application();
    ~Application();

    bool Initialize(int windowWidth = 680, int windowHeight = 1080, const char* windowTitle = "Symulacja Dymu");
    void Run();
    void Clean();

    [[nodiscard]] int getMaxFps() const { return m_maxFps; }
    void setMaxFps(int fps) { m_maxFps = (fps >= 0) ? fps : 0; }

private:
    GLFWwindow* m_window;
    Renderer m_renderer;
    Camera m_camera;
    SmokeSimulation m_simulation;
    
    int m_windowWidth;
    int m_windowHeight;
    bool m_initialized;
    
    float m_lastFrameTime;
    float m_lastRenderTime;

    int m_maxFps{ 0 };

    bool m_showTempMode{ false };
    bool m_campfireWireframe{ false };
    int m_prevF1{ GLFW_RELEASE };
    int m_prevF2{ GLFW_RELEASE };

    bool m_firstMouse;
    float m_lastMouseX;
    float m_lastMouseY;
    
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    
    bool initializeGLFW();
    bool initializeOpenGL();
    bool initializeImGui();
    bool initializeSimulation();
    void updateFrame(float deltaTime);
    void renderFrame();
    void processInput(float deltaTime);
};
