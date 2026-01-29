#pragma once

#include "Shader.h"
#include "GltfModel.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

// Forward declaration
class Grid;

// Struktura reprezentująca cząsteczkę gazu
struct GasParticle {
    glm::vec3 position;      // Pozycja [m]
    glm::vec3 velocity;      // Prędkość [m/s]
    float density;            // Gęstość [g/cm^3]
    float pressure;           // Ciśnienie [hPa]
    float size;               // Rozmiar cząsteczki do renderowania
    bool toErase;
};

struct GPUParticle {
    glm::vec3 position;
        float _pad1;        // std430 alignment
    glm::vec3 velocity;
    float pressure;
};

struct SimParamsGPU {
    float deltaTime;
    float gravity;
    float ambientPressure;
    float dampingFactor;

    float simulationHeight;
    float cellSize;
    float maxVelocity;
    uint32_t totalCells;

    glm::vec3 boundsMin;
    float pad0;

    glm::vec3 boundsMax;
    float pad1;
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    // Inicjalizacja renderera
    bool initialize(int windowWidth, int windowHeight);
    
    // Renderuje cząsteczki gazu
    void renderGasParticles(const std::vector<GasParticle>& particles);
    
    // Renderuje przeszkody (ściany)
    void renderObstacles(const std::vector<glm::vec4>& obstacles); // x, y, width, height
    
    // Renderuje kontury grid'a
    void renderGridWireframe(const class Grid& grid);
    
    // Ustawia macierz projekcji
    void setProjection(const glm::mat4& projection);
    
    // Ustawia macierz widoku
    void setView(const glm::mat4& view);
    
    // Czyszczenie ekranu
    void clear(float r = 0.1f, float g = 0.1f, float b = 0.1f, float a = 1.0f);

    // Campfire (spawner) – ładowanie i render
    bool loadCampfire(const std::string& path);
    void renderCampfire(const glm::vec3& position) const;

    static std::string findCampfirePath();
    
private:
    Shader m_particleShader;
    Shader m_obstacleShader;
    Shader m_gridShader;
    Shader m_modelShader;
    GltfModel m_campfire;
    
    GLuint m_particleVAO;
    GLuint m_particleVBO;
    GLuint m_obstacleVAO;
    GLuint m_obstacleVBO;
    GLuint m_gridVAO;
    GLuint m_gridVBO;
    
    glm::mat4 m_projection;
    glm::mat4 m_view;
    std::vector<float> m_particleDataBuffer;

    bool m_initialized;

    // Inicjalizuje VAO/VBO dla cząsteczek
    void initParticleBuffers();
    // Inicjalizuje VAO/VBO dla przeszkód
    void initObstacleBuffers();
    // Inicjalizuje VAO/VBO dla grid'a
    void initGridBuffers();

    GLuint createComputeProgram(const char* compPath);
};
