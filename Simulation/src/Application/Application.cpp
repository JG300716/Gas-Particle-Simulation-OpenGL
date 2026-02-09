#include "Application.h"
#include "../Simulation/Logger.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <thread>

#include "../../../ImGui/imgui.h"
#include "../../../ImGui/imgui_impl_glfw.h"
#include "../../../ImGui/imgui_impl_opengl3.h"

Application::Application() 
    : m_window(nullptr), 
      m_windowWidth(680), 
      m_windowHeight(420), 
      m_initialized(false),
      m_firstMouse(true),
      m_lastMouseX(0.0f),
      m_lastMouseY(0.0f),
      m_lastFrameTime(0.0f),
      m_lastRenderTime(0.0f) {
}

Application::~Application() {
    Clean();
}

bool Application::Initialize(int windowWidth, int windowHeight, const char* windowTitle) {
    m_windowWidth = windowWidth;
    m_windowHeight = windowHeight;

    Logger::init("");

    // Inicjalizacja GLFW
    if (!initializeGLFW()) {
        return false;
    }
    
    // Utworzenie okna
    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, windowTitle, nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(m_window);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);
    glfwSetCursorPosCallback(m_window, mouse_callback);
    glfwSetScrollCallback(m_window, scroll_callback);
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // Inicjalizacja OpenGL/GLAD
    if (!initializeOpenGL()) {
        glfwDestroyWindow(m_window);
        glfwTerminate();
        return false;
    }
    
    // Inicjalizacja ImGui
    if (!initializeImGui()) {
        glfwDestroyWindow(m_window);
        glfwTerminate();
        return false;
    }
    
    // Inicjalizacja renderera i symulacji
    if (!initializeSimulation()) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(m_window);
        glfwTerminate();
        return false;
    }
    
    m_initialized = true;
    return true;
}

bool Application::initializeGLFW() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    // Konfiguracja GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    return true;
}

bool Application::initializeOpenGL() {
    // Inicjalizacja GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }
    
    // Konfiguracja OpenGL
    glViewport(0, 0, m_windowWidth, m_windowHeight);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    return true;
}

bool Application::initializeImGui() {
    // Inicjalizacja ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    
    // Setup Platform/Renderer backends
    if (!ImGui_ImplGlfw_InitForOpenGL(m_window, true)) {
        std::cerr << "Failed to initialize ImGui GLFW backend" << std::endl;
        return false;
    }
    
    if (!ImGui_ImplOpenGL3_Init("#version 460")) {
        std::cerr << "Failed to initialize ImGui OpenGL3 backend" << std::endl;
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        return false;
    }
    
    return true;
}

bool Application::initializeSimulation() {
    if (!m_renderer.initialize(m_windowWidth, m_windowHeight)) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return false;
    }
    
    if (!m_simulation.initialize(20, 20, 20)) {
        std::cerr << "Failed to initialize simulation" << std::endl;
        return false;
    }
    m_simulation.setSpawnerPosition(glm::vec3(0.f, -8.f, 0.f));

    m_camera.setPosition(glm::vec3(0.0f, 8.0f, 40.0f));
    m_camera.rotate(-90.0f, -15.0f);
    m_camera.setNearFar(0.1f, 500.0f);
    
    return true;
}

void Application::Run() {
    if (!m_initialized) {
        std::cerr << "Application not initialized. Call Initialize() first." << std::endl;
        return;
    }
    
    float lastTime = static_cast<float>(glfwGetTime());
    
    while (!glfwWindowShouldClose(m_window)) {
        auto frameStart = std::chrono::high_resolution_clock::now();
        
        // Oblicz deltaTime
        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        
        // Obsługa zdarzeń
        glfwPollEvents();
        
        // Zmierz czas renderowania
        auto renderStart = std::chrono::high_resolution_clock::now();
        
        // Aktualizacja i renderowanie
        updateFrame(deltaTime);
        renderFrame();
        
        // Zmierz czas renderowania
        auto renderEnd = std::chrono::high_resolution_clock::now();
        float renderTime = std::chrono::duration<float, std::milli>(renderEnd - renderStart).count();
        m_lastRenderTime = renderTime;
        m_lastFrameTime = deltaTime * 1000.0f; // Konwersja do ms
        
        // Zamiana buforów
        glfwSwapBuffers(m_window);
        
        // Limit FPS (0 = bez limitu)
        if (m_maxFps > 0) {
            auto frameEnd = std::chrono::high_resolution_clock::now();
            double frameElapsed = std::chrono::duration<double>(frameEnd - frameStart).count();
            double minFrameTime = 1.0 / static_cast<double>(m_maxFps);
            if (frameElapsed < minFrameTime) {
                std::this_thread::sleep_for(
                    std::chrono::duration<double>(minFrameTime - frameElapsed));
            }
        }
    }
}

void Application::updateFrame(float deltaTime) {
    // Rozpocznij nową ramkę ImGui
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    // Uruchom symulację
    m_simulation.run(deltaTime);
    
    // Renderuj UI (wszystko w jednym oknie)
    ImGuiControls::renderAllControls(m_simulation, m_camera, m_renderer, this);
    
    // Renderuj overlay z wydajnością
    ImGuiControls::renderPerformanceOverlay(m_lastFrameTime, m_lastRenderTime);
    
    // Obsługa klawiatury dla kamery
    processInput(deltaTime);
}

void Application::renderFrame() {
    // Aktualizuj macierze kamery
    float aspect = static_cast<float>(m_windowWidth) / static_cast<float>(m_windowHeight);
    m_renderer.setProjection(m_camera.getProjectionMatrix(aspect));
    m_renderer.setView(m_camera.getViewMatrix());
    
    // Renderuj scenę
    m_renderer.clear(0.1f, 0.1f, 0.15f, 1.0f);
    
    std::vector<ObstacleDesc> obstaclesForRender;
    for (const auto& o : m_simulation.getObstacles()) {
        obstaclesForRender.push_back({ o.position, o.size, o.rotation, o.scale });
    }
    m_renderer.renderObstacles(obstaclesForRender);

    m_renderer.renderCampfire(m_simulation.getSpawnerPosition(), m_campfireWireframe);

    m_renderer.renderLightIndicator();

    m_renderer.renderGridWireframe(m_simulation.getGrid());

    m_renderer.renderSmokeVolume(m_simulation, m_showTempMode);
    
    // Renderuj ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Application::Clean() {
    if (!m_initialized) {
        return;
    }

    Logger::shutdown();

    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    // Cleanup GLFW
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
    
    m_initialized = false;
}

void Application::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    
    // Zaktualizuj rozmiar okna w aplikacji
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->m_windowWidth = width;
        app->m_windowHeight = height;
    }
}

void Application::mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (!app) return;
    
    // Nie obracaj kamery jeśli ESC jest wciśnięty
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        return;
    }
    
    if (app->m_firstMouse) {
        app->m_lastMouseX = static_cast<float>(xpos);
        app->m_lastMouseY = static_cast<float>(ypos);
        app->m_firstMouse = false;
    }
    
    float xoffset = static_cast<float>(xpos) - app->m_lastMouseX;
    float yoffset = app->m_lastMouseY - static_cast<float>(ypos); // Odwrócone, bo y idzie od góry do dołu
    
    app->m_lastMouseX = static_cast<float>(xpos);
    app->m_lastMouseY = static_cast<float>(ypos);
    
    // Obracanie kamery tylko jeśli kursor jest zablokowany
    if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
        const float sensitivity = 0.1f;
        app->m_camera.addRotation(xoffset * sensitivity, yoffset * sensitivity);
    }
}

void Application::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (!app) return;
    
    // Przybliżanie/oddalanie kamery przez przesuwanie pozycji wzdłuż forward vector
    glm::vec3 pos = app->m_camera.getPosition();
    glm::vec3 forward = app->m_camera.getForward();
    
    float scrollSpeed = 0.5f;
    pos += forward * static_cast<float>(yoffset) * scrollSpeed;
    
    app->m_camera.setPosition(pos);
}

void Application::processInput(float deltaTime) {
    int f1 = glfwGetKey(m_window, GLFW_KEY_F1);
    int f2 = glfwGetKey(m_window, GLFW_KEY_F2);
    if (f1 == GLFW_PRESS && m_prevF1 != GLFW_PRESS) m_campfireWireframe = !m_campfireWireframe;
    if (f2 == GLFW_PRESS && m_prevF2 != GLFW_PRESS) m_showTempMode = !m_showTempMode;
    m_prevF1 = f1;
    m_prevF2 = f2;

    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        // Przełącz tryb kursora
        int mode = glfwGetInputMode(m_window, GLFW_CURSOR);
        glfwSetInputMode(m_window, GLFW_CURSOR, 
                       mode == GLFW_CURSOR_DISABLED ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
        m_firstMouse = true;
    }
    
    // Ruch kamery (tylko gdy kursor jest zablokowany)
    if (glfwGetInputMode(m_window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
        const float cameraSpeed = 5.0f * deltaTime;
        glm::vec3 pos = m_camera.getPosition();
        glm::vec3 forward = m_camera.getForward();
        glm::vec3 right = m_camera.getRight();
        
        // W/S - tylko forward/backward (bez zmiany wysokości)
        glm::vec3 forwardHorizontal = glm::normalize(glm::vec3(forward.x, 0.0f, forward.z));
        
        if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
            pos += forwardHorizontal * cameraSpeed;
        if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
            pos -= forwardHorizontal * cameraSpeed;
        if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
            pos -= right * cameraSpeed;
        if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
            pos += right * cameraSpeed;
        
        // Q/E - góra/dół
        if (glfwGetKey(m_window, GLFW_KEY_Q) == GLFW_PRESS)
            pos += glm::vec3(0.0f, 1.0f, 0.0f) * cameraSpeed;
        if (glfwGetKey(m_window, GLFW_KEY_E) == GLFW_PRESS)
            pos -= glm::vec3(0.0f, 1.0f, 0.0f) * cameraSpeed;
        
        m_camera.setPosition(pos);
    }
}
