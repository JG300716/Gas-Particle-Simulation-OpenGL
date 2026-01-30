#pragma once

#include "Grid.h"
#include <vector>
#include <glm/glm.hpp>
#include <basetsd.h>

struct Obstacle {
    glm::vec3 position;
    glm::vec3 size;
};

// Symulacja dymu w stylu Bridsona (SIGGRAPH 2007): siatka Eulerowska,
// semi-Lagrange adwekcja, buoyancy, pressure projection. Por. Seb Lague, fluids_notes.pdf.
class SmokeSimulation {
public:
    SmokeSimulation();
    ~SmokeSimulation();

    bool initialize(const UINT32& simulationWidth = 40, const UINT32& simulationHeight = 40,
                    const UINT32& simulationDepth = 40, const float& cellSize = 1.0f);

    void run(float deltaTime);

    // Parametry
    float getAmbientPressure() const { return m_ambientPressure; }
    void setAmbientPressure(float v) { m_ambientPressure = v; }
    float getGravity() const { return m_gravity; }
    void setGravity(float v) { m_gravity = v; }
    float getCellSize() const { return m_cellSize; }
    void setCellSize(float v);
    float getTimeScale() const { return m_timeScale; }
    void setTimeScale(float v) { m_timeScale = std::max(0.0f, v); }
    float getTempAmbient() const { return m_Tamb; }
    void setTempAmbient(float v) { m_Tamb = v; }
    float getBuoyancyAlpha() const { return m_buoyancyAlpha; }
    void setBuoyancyAlpha(float v) { m_buoyancyAlpha = v; }
    float getBuoyancyBeta() const { return m_buoyancyBeta; }
    void setBuoyancyBeta(float v) { m_buoyancyBeta = v; }
    float getDissipation() const { return m_dissipation; }
    void setDissipation(float v) { m_dissipation = std::max(0.f, std::min(2.f, v)); }

    void setGridSize(int sizeX, int sizeY, int sizeZ);
    int getGridSizeX() const { return m_grid.getSizeX(); }
    int getGridSizeY() const { return m_grid.getSizeY(); }
    int getGridSizeZ() const { return m_grid.getSizeZ(); }

    glm::vec3 getSpawnerPosition() const { return m_spawnerPosition; }
    void setSpawnerPosition(const glm::vec3& p) { m_spawnerPosition = p; }

    void addObstacle(const Obstacle& o);
    void removeObstacle(size_t i);
    void clearObstacles();
    const std::vector<Obstacle>& getObstacles() const { return m_obstacles; }

    const Grid& getGrid() const { return m_grid; }

    // Siatka dymu dla renderera (raymarch 3D)
    const float* getSmokeDensityData() const { return m_s.data(); }
    int getSmokeNx() const { return m_nx; }
    int getSmokeNy() const { return m_ny; }
    int getSmokeNz() const { return m_nz; }
    int getSmokeCellCount() const;

private:
    Grid m_grid;
    std::vector<Obstacle> m_obstacles;
    glm::vec3 m_spawnerPosition{ 0.f, -50.f, 0.f };

    int m_nx{ 0 }, m_ny{ 0 }, m_nz{ 0 };
    float m_cellSize{ 1.0f };
    float m_ambientPressure{ 1013.25f };
    float m_gravity{ 9.81f };
    float m_timeScale{ 1.0f };
    float m_Tamb{ 0.f };
    float m_buoyancyAlpha{ 0.1f };
    float m_buoyancyBeta{ 0.2f };
    float m_dissipation{ 0.15f };
    float m_injectRate{ 8.f };
    int m_injectRadius{ 2 };

    std::vector<float> m_u, m_v, m_w, m_p, m_s, m_T;
    std::vector<float> m_uTmp, m_vTmp, m_wTmp, m_sTmp, m_TTmp, m_pTmp;
    std::vector<uint8_t> m_solid;

    int flidx(int i, int j, int k) const { return i + j * m_nx + k * m_nx * m_ny; }
    glm::vec3 cellCenter(int i, int j, int k) const;
    bool isSolid(int i, int j, int k) const;
    bool isPointInObstacle(const glm::vec3& p) const;

    void buildSolidMask();
    void allocFluid();
    void runFluid(float dt);
    void injectSmoke(float dt);
    void dissipate(float dt);
    void addBuoyancyGravity(float dt);
    void advect(float dt);
    void pressureSolve(int iterations);
    void project();

    float sampleTrilinear(const std::vector<float>& f, float cx, float cy, float cz) const;
};
