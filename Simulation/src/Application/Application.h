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

    // Inicjalizacja aplikacji
    bool Initialize(int windowWidth = 1280, int windowHeight = 720, const char* windowTitle = "Symulacja Dymu");
    
    // Uruchomienie głównej pętli
    void Run();
    
    // Czyszczenie zasobów
    void Clean();

private:
    GLFWwindow* m_window;
    Renderer m_renderer;
    Camera m_camera;
    SmokeSimulation m_simulation;
    
    int m_windowWidth;
    int m_windowHeight;
    bool m_initialized;
    
    // Performance tracking
    float m_lastFrameTime;
    float m_lastRenderTime;
    
    // Kontrola kamery
    bool m_firstMouse;
    float m_lastMouseX;
    float m_lastMouseY;
    
    // Callback dla zmiany rozmiaru okna
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    // Callback dla myszy
    static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
    // Callback dla scrolla
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    
    // Inicjalizacja GLFW
    bool initializeGLFW();
    
    // Inicjalizacja OpenGL/GLAD
    bool initializeOpenGL();
    
    // Inicjalizacja ImGui
    bool initializeImGui();
    
    // Inicjalizacja renderera i symulacji
    bool initializeSimulation();
    
    // Aktualizacja jednej ramki
    void updateFrame(float deltaTime);
    
    // Renderowanie jednej ramki
    void renderFrame();
    
    // Obsługa wejścia
    void processInput(float deltaTime);
};
