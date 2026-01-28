#include "Application.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <algorithm>
#include <chrono>

#include "../../../ImGui/imgui.h"
#include "../../../ImGui/imgui_impl_glfw.h"
#include "../../../ImGui/imgui_impl_opengl3.h"

Application::Application() 
    : m_window(nullptr), 
      m_windowWidth(1280), 
      m_windowHeight(720), 
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
    
    if (!m_simulation.initialize(100.0f, 100.0f, 100.0f, 1.0f)) {
        std::cerr << "Failed to initialize simulation" << std::endl;
        return false;
    }
    
    // Oddalona kamera dla lepszego widoku grid boxa (100x100x100)
    // Pozycja wyżej i dalej, żeby zobaczyć cały grid
    m_camera.setPosition(glm::vec3(0.0f, 80.0f, 250.0f));
    m_camera.rotate(-90.0f, -15.0f); // Lekko w dół, żeby patrzeć na grid
    m_camera.setNearFar(0.1f, 2000.0f); // Zwiększony far plane dla większych grid'ów
    
    return true;
}

void Application::Run() {
    if (!m_initialized) {
        std::cerr << "Application not initialized. Call Initialize() first." << std::endl;
        return;
    }
    
    float lastTime = static_cast<float>(glfwGetTime());
    
    while (!glfwWindowShouldClose(m_window)) {
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
    ImGuiControls::renderAllControls(m_simulation, m_camera);
    
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
    
    // Renderuj przeszkody (konwersja do vec4: x, y, width, height)
    std::vector<glm::vec4> obstaclesForRender;
    for (const auto& obstacle : m_simulation.getObstacles()) {
        obstaclesForRender.push_back(glm::vec4(
            obstacle.position.x - obstacle.size.x * 0.5f,
            obstacle.position.y - obstacle.size.y * 0.5f,
            obstacle.size.x,
            obstacle.size.y
        ));
    }
    m_renderer.renderObstacles(obstaclesForRender);
    
    // Renderuj kontury grid'a
    m_renderer.renderGridWireframe(m_simulation.getGrid());
    
    // Renderuj cząsteczki gazu
    m_renderer.renderGasParticles(m_simulation.getGasParticles());
    
    // Renderuj ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Application::Clean() {
    if (!m_initialized) {
        return;
    }
    
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
