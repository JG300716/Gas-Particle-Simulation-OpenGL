#pragma once

#include "../Render/Renderer.h"
#include "Grid.h"
#include <vector>
#include <glm/glm.hpp>
#include <basetsd.h>
#include <glad/glad.h>

// Przeszkoda (ściana) - w 3D
struct Obstacle {
    glm::vec3 position;   // Pozycja środka
    glm::vec3 size;       // Rozmiar (szerokość, wysokość, głębokość)
};

class SmokeSimulation {
public:
    SmokeSimulation();
    ~SmokeSimulation();

    // Inicjalizacja symulacji
    bool initialize(const UINT32 &simulationWidth = 40, const UINT32 &simulationHeight = 40, const UINT32 &simulationDepth = 40, const float &cellSize = 1.0f);
    
    // Uruchomienie jednej iteracji symulacji
    void run(float deltaTime);
    
    // Pobierz cząsteczki gazu
    const std::vector<GasParticle>& getGasParticles() const { return m_gasParticles; }
    
    // Parametry symulacji
    float getAmbientPressure() const { return m_ambientPressure; }
    void setAmbientPressure(const float &pressure) { m_ambientPressure = pressure; }
    
    float getParticleDensity() const { return m_particleDensity; }
    void setParticleDensity(const float &density) { m_particleDensity = density; }
    
    float getGravity() const { return m_gravity; }
    void setGravity(const float &gravity) { m_gravity = gravity; }

    float getDampingFactor() const { return m_dampingFactor; }
    void setDampingFactor(const float &factor) { m_dampingFactor = factor; }

    float getCellSize() const { return m_cellSize; }
    void setCellSize(const float &size) { m_cellSize = std::max(0.01f, size); }
    
    // Time scale
    float getTimeScale() const { return m_timeScale; }
    void setTimeScale(const float &timeScale) { m_timeScale = std::max(0.0f, timeScale); }
    
    // Grid size
    void setGridSize(int sizeX, int sizeY, int sizeZ);
    int getGridSizeX() const { return m_grid.getSizeX(); }
    int getGridSizeY() const { return m_grid.getSizeY(); }
    int getGridSizeZ() const { return m_grid.getSizeZ(); }
    
    // Spawner cząsteczek
    glm::vec3 getSpawnerPosition() const { return m_spawnerPosition; }
    void setSpawnerPosition(const glm::vec3& position) { m_spawnerPosition = position; }
    void spawnParticles(int count = 10);
    
    // Zarządzanie przeszkodami
    void addObstacle(const Obstacle& obstacle);
    void removeObstacle(size_t index);
    const std::vector<Obstacle>& getObstacles() const { return m_obstacles; }
    void clearObstacles();
    
    // Dodaj cząsteczki w określonym miejscu
    void addParticlesAt(const glm::vec3& position, int count = 10);
    
    // Pobierz grid
    const Grid& getGrid() const { return m_grid; }

private:
    std::vector<GasParticle> m_gasParticles;
    std::vector<Obstacle> m_obstacles;
    Grid m_grid;
    
    // Occupancy: płaski wektor [0..totalCells-1], EMPTY = wolna komórka (1 cząsteczka na pole)
    static constexpr size_t OCCUPANCY_EMPTY = static_cast<size_t>(-1);
    std::vector<size_t> m_gridOccupancyFlat;
    int m_strideY{ 0 };   // sizeX
    int m_strideZ{ 0 };   // sizeX * sizeY
    int m_totalCells{ 0 };
    
    float m_ambientPressure;      // Globalne ciśnienie otoczenia [hPa]
    float m_particleDensity;       // Gęstość cząsteczki gazu [g/cm^3]
    float m_gravity;               // Grawitacja [m/s^2]
    float m_timeScale;             // Modyfikator czasu (0.0 = zatrzymane, 1.0 = normalna prędkość, >1.0 = przyspieszone)
    float m_dampingFactor;        // Tłumienie prędkości cząsteczek
    float m_cellSize{ 1.0f };          // Rozmiar komórki grid'a
    
    int m_simulationWidth;
    int m_simulationHeight;
    int m_simulationDepth;
    
    glm::vec3 m_spawnerPosition;   // Pozycja spawnera cząsteczek
    
    // Fizyka
    void updateParticles(float deltaTime);
    void checkCollisions();
    void applyParticleInteractions();
    bool isPointInObstacle(const glm::vec3& point) const;
    glm::vec3 getCollisionNormal(const glm::vec3& point, const Obstacle& obstacle) const;
    
    // Grid operations
    int gridPosToKey(const glm::ivec3& gridPos) const;
    glm::ivec3 keyToGridPos(int key) const;
    bool canPlaceParticleAt(const glm::ivec3& gridPos) const;
    void updateGridOccupancy();
    glm::ivec3 findFreeNeighborCell(const glm::ivec3& gridPos, const glm::vec3& velocity) const; // Znajdź wolną komórkę obok
    
    // Generowanie początkowych cząsteczek
    void generateInitialParticles();

    void updateGridStrides();

    // GPU compute shader
    GLuint m_computeProgram{ 0 };
    GLuint m_particleSSBO{ 0 };
    GLuint m_occupancySSBO{ 0 };
    GLuint m_paramsUBO{ 0 };
    bool m_useGPU{ true };

    bool initComputeBuffers();
    void updateComputeBuffers();
    void updateParticlesGPU(float deltaTime);
    void cleanupComputeBuffers();
};
