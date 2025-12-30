#include <iostream>
#include <string>

// Test 1: GLAD
#ifdef _WIN32
#include <Windows.h>
#endif

#ifndef GLAD_H_
    #include <glad/glad.h>
#endif

// Test 2: GLFW
#ifndef GLFW_VERSION_MAJOR
    #include <GLFW/glfw3.h>
#endif

// Test 3: GLM
#ifndef GLM_VERSION
    #include <glm/glm.hpp>
    #include <glm/gtc/matrix_transform.hpp>
    #include <glm/gtc/type_ptr.hpp>
#endif

// Test 4: ImGui
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_impl_glfw.h"
#include "../ImGui/imgui_impl_opengl3.h"

// ============================================================================
// Kolory w konsoli (opcjonalne, dla ładniejszego wyświetlania)
// ============================================================================
enum ConsoleColor {
    RED = 12,
    GREEN = 10,
    YELLOW = 14,
    CYAN = 11,
    WHITE = 7
};

void setColor(ConsoleColor color) {
#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
#endif
}

void printTest(const std::string& testName, bool passed) {
    std::cout << "[";
    if (passed) {
        setColor(GREEN);
        std::cout << "✓";
        setColor(WHITE);
        std::cout << "] ";
    } else {
        setColor(RED);
        std::cout << "✗";
        setColor(WHITE);
        std::cout << "] ";
    }
    std::cout << testName << std::endl;
}

// ============================================================================
// Testy bibliotek
// ============================================================================

bool testGLAD() {
    // GLAD załaduje się dopiero po stworzeniu kontekstu OpenGL
    // Tutaj tylko sprawdzamy czy się kompiluje
    return true;
}

bool testGLFW() {
    if (!glfwInit()) {
        return false;
    }
    
    // Sprawdź wersję
    int major, minor, revision;
    glfwGetVersion(&major, &minor, &revision);
    std::cout << "    GLFW Version: " << major << "." << minor << "." << revision << std::endl;
    
    glfwTerminate();
    return true;
}

bool testGLM() {
    // Test podstawowych operacji
    glm::vec3 vec(1.0f, 2.0f, 3.0f);
    glm::mat4 mat = glm::mat4(1.0f);
    
    // Sprawdź czy operacje działają
    glm::vec3 result = glm::vec3(mat * glm::vec4(vec, 1.0f));
    
    std::cout << "    GLM Version: " << GLM_VERSION_MAJOR << "." 
              << GLM_VERSION_MINOR << "." << GLM_VERSION_PATCH << std::endl;
    std::cout << "    Test Vector: (" << result.x << ", " << result.y << ", " << result.z << ")" << std::endl;
    
    return (result.x == vec.x && result.y == vec.y && result.z == vec.z);
}

bool testImGui() {
    // Sprawdź wersję ImGui
    std::cout << "    ImGui Version: " << IMGUI_VERSION << std::endl;
    
    // Test podstawowej funkcjonalności - tworzenie kontekstu
    ImGuiContext* ctx = ImGui::CreateContext();
    if (!ctx) {
        std::cerr << "    ERROR: Failed to create ImGui context" << std::endl;
        return false;
    }
    
    // Test podstawowych funkcji
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1280, 720);
    
    std::cout << "    ImGui Context created successfully" << std::endl;
    std::cout << "    Display Size: " << io.DisplaySize.x << "x" << io.DisplaySize.y << std::endl;
    
    // Cleanup
    ImGui::DestroyContext(ctx);
    
    return true;
}

bool testOpenGLContext() {
    // Inicjalizacja GLFW
    if (!glfwInit()) {
        std::cerr << "    ERROR: Failed to initialize GLFW" << std::endl;
        return false;
    }

    // Ustawienia okna (niewidoczne, tylko do testu)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Ukryte okno

    // Stwórz okno
    GLFWwindow* window = glfwCreateWindow(800, 600, "Library Test", nullptr, nullptr);
    if (!window) {
        std::cerr << "    ERROR: Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);

    // Test GLAD - załaduj funkcje OpenGL
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "    ERROR: Failed to initialize GLAD" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return false;
    }

    // Wyświetl informacje OpenGL
    std::cout << "    OpenGL Version:  " << glGetString(GL_VERSION) << std::endl;
    std::cout << "    GPU Vendor:      " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "    GPU Renderer:    " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "    GLSL Version:    " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    // Test podstawowej funkcji OpenGL
    GLint maxTextureSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    std::cout << "    Max Texture Size: " << maxTextureSize << "x" << maxTextureSize << std::endl;

    // Cleanup
    glfwDestroyWindow(window);
    glfwTerminate();

    return true;
}

bool testImGuiWithOpenGL() {
    // Inicjalizacja GLFW
    if (!glfwInit()) {
        std::cerr << "    ERROR: Failed to initialize GLFW" << std::endl;
        return false;
    }

    // Ustawienia okna
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Ukryte okno

    GLFWwindow* window = glfwCreateWindow(800, 600, "ImGui Test", nullptr, nullptr);
    if (!window) {
        std::cerr << "    ERROR: Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    
    // Inicjalizacja GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "    ERROR: Failed to initialize GLAD" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return false;
    }

    // Inicjalizacja ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    
    // Setup Platform/Renderer backends
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        std::cerr << "    ERROR: Failed to initialize ImGui GLFW backend" << std::endl;
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        return false;
    }
    
    if (!ImGui_ImplOpenGL3_Init("#version 460")) {
        std::cerr << "    ERROR: Failed to initialize ImGui OpenGL3 backend" << std::endl;
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        return false;
    }

    std::cout << "    ImGui GLFW backend initialized" << std::endl;
    std::cout << "    ImGui OpenGL3 backend initialized" << std::endl;
    
    // Test renderowania pojedynczej ramki
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    // Stwórz przykładowe okno ImGui
    ImGui::Begin("Test Window");
    ImGui::Text("This is a test window");
    ImGui::End();
    
    ImGui::Render();
    
    std::cout << "    ImGui test frame rendered successfully" << std::endl;

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return true;
}

// ============================================================================
// Main - Uruchom wszystkie testy
// ============================================================================

int main() {
    setColor(CYAN);
    std::cout << "========================================" << std::endl;
    std::cout << "  PWAG - Library Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    setColor(WHITE);
    std::cout << std::endl;

    int passedTests = 0;
    int totalTests = 0;

    // Test 1: GLAD (kompilacja)
    std::cout << "Testing GLAD..." << std::endl;
    bool gladTest = testGLAD();
    printTest("GLAD Headers", gladTest);
    if (gladTest) passedTests++;
    totalTests++;
    std::cout << std::endl;

    // Test 2: GLFW
    std::cout << "Testing GLFW..." << std::endl;
    bool glfwTest = testGLFW();
    printTest("GLFW Initialization", glfwTest);
    if (glfwTest) passedTests++;
    totalTests++;
    std::cout << std::endl;

    // Test 3: GLM
    std::cout << "Testing GLM..." << std::endl;
    bool glmTest = testGLM();
    printTest("GLM Math Operations", glmTest);
    if (glmTest) passedTests++;
    totalTests++;
    std::cout << std::endl;

    // Test 4: ImGui (podstawowy)
    std::cout << "Testing ImGui..." << std::endl;
    bool imguiTest = testImGui();
    printTest("ImGui Context Creation", imguiTest);
    if (imguiTest) passedTests++;
    totalTests++;
    std::cout << std::endl;

    // Test 5: OpenGL Context + GLAD loading
    std::cout << "Testing OpenGL Context & GLAD Loading..." << std::endl;
    bool openglTest = testOpenGLContext();
    printTest("OpenGL Context Creation", openglTest);
    if (openglTest) passedTests++;
    totalTests++;
    std::cout << std::endl;

    // Test 6: ImGui z OpenGL (pełna integracja)
    std::cout << "Testing ImGui with OpenGL..." << std::endl;
    bool imguiOpenGLTest = testImGuiWithOpenGL();
    printTest("ImGui + OpenGL Integration", imguiOpenGLTest);
    if (imguiOpenGLTest) passedTests++;
    totalTests++;
    std::cout << std::endl;

    // Podsumowanie
    setColor(CYAN);
    std::cout << "========================================" << std::endl;
    std::cout << "  Test Results" << std::endl;
    std::cout << "========================================" << std::endl;
    setColor(WHITE);
    
    std::cout << "Passed: ";
    setColor(passedTests == totalTests ? GREEN : YELLOW);
    std::cout << passedTests << "/" << totalTests;
    setColor(WHITE);
    std::cout << std::endl;

    if (passedTests == totalTests) {
        setColor(GREEN);
        std::cout << "\n✓ All libraries loaded successfully!" << std::endl;
        std::cout << "✓ Your project is ready to build!" << std::endl;
        setColor(WHITE);
    } else {
        setColor(RED);
        std::cout << "\n✗ Some libraries failed to load!" << std::endl;
        std::cout << "✗ Check the errors above for details." << std::endl;
        setColor(WHITE);
    }

    std::cout << "\n========================================" << std::endl;

#ifdef _DEBUG
    std::cout << "\nPress Enter to exit...";
    std::cin.get();
#endif

    return (passedTests == totalTests) ? 0 : 1;
}