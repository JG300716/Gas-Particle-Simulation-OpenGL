#pragma once

#include "Grid.h"
#include <vector>
#include <glm/glm.hpp>
#include <basetsd.h>

struct Obstacle {
    glm::vec3 position{ 0.f };
    glm::vec3 size{ 2.f };
    glm::vec3 rotation{ 0.f };
    glm::vec3 scale{ 1.f };
};

// Ściana z dziurą (np. sufit) - obrócona płaska przeszkoda z prostokątnym otworem
struct WallWithHole {
    glm::vec3 position{ 0.f };       // środek ściany
    glm::vec3 normal{ 0.f, -1.f, 0.f }; // kierunek "na zewnątrz" (domyślnie sufit patrzy w dół)
    float width{ 20.f };             // szerokość ściany (X)
    float depth{ 20.f };             // głębokość ściany (Z)
    float thickness{ 1.f };          // grubość ściany
    glm::vec2 holeCenter{ 0.f, 0.f }; // środek dziury w lokalnych XZ
    glm::vec2 holeSize{ 4.f, 4.f };   // rozmiar dziury
    bool enabled{ false };
};

// Symulacja dymu w stylu Bridsona (SIGGRAPH 2007): siatka Eulerowska,
// semi-Lagrange adwekcja, buoyancy, pressure projection. Por. Seb Lague, fluids_notes.pdf.
class SmokeSimulation {
public:
    SmokeSimulation();
    ~SmokeSimulation();

    bool initialize(const UINT32& simulationWidth = 40, const UINT32& simulationHeight = 40,
                    const UINT32& simulationDepth = 40);

    void run(float deltaTime);

    // Parametry
    float getAmbientPressure() const { return m_ambientPressure; }
    void setAmbientPressure(float v) { m_ambientPressure = v; }
    float getGravity() const { return m_gravity; }
    void setGravity(float v) { m_gravity = v; }
    float getTimeScale() const { return m_timeScale; }
    void setTimeScale(float v) { m_timeScale = std::max(0.0f, v); }
    float getTempAmbient() const { return m_Tamb; }
    void setTempAmbient(float v) { m_Tamb = v; }
    float getBuoyancyAlpha() const { return m_buoyancyAlpha; }
    void setBuoyancyAlpha(float v) { m_buoyancyAlpha = v; }
    float getBuoyancyBeta() const { return m_buoyancyBeta; }
    void setBuoyancyBeta(float v) { m_buoyancyBeta = v; }
    float getTempCooling() const { return m_tempCooling; }
    void setTempCooling(float v) { m_tempCooling = std::max(0.f, std::min(10.f, v)); }
    float getDiffusion() const { return m_diffusion; }
    void setDiffusion(float v) { m_diffusion = std::max(0.f, std::min(10.f, v)); }
    float getInjectRate() const { return m_injectRate; }
    void setInjectRate(float v) { m_injectRate = std::max(0.f, std::min(50.f, v)); }
    bool getInjectCylinder() const { return m_injectCylinder; }
    void setInjectCylinder(bool v) { m_injectCylinder = v; }

    void setGridSize(int sizeX, int sizeY, int sizeZ);
    int getGridSizeX() const { return m_grid.getSizeX(); }
    int getGridSizeY() const { return m_grid.getSizeY(); }
    int getGridSizeZ() const { return m_grid.getSizeZ(); }

    glm::vec3 getSpawnerPosition() const { return m_spawnerPosition; }
    void setSpawnerPosition(const glm::vec3& p) { m_spawnerPosition = p; }

    void addObstacle(const Obstacle& o);
    void updateObstacle(size_t i, const Obstacle& o);
    void removeObstacle(size_t i);
    void clearObstacles();
    const std::vector<Obstacle>& getObstacles() const { return m_obstacles; }

    // Sufit z dziurą
    WallWithHole& getCeiling() { return m_ceiling; }
    const WallWithHole& getCeiling() const { return m_ceiling; }
    void setCeilingEnabled(bool enabled);
    void setCeilingHole(const glm::vec2& center, const glm::vec2& size);
    void updateCeiling();

    const Grid& getGrid() const { return m_grid; }

    // Siatka dymu dla renderera (raymarch 3D)
    const float* getSmokeDensityData() const { return m_s.data(); }
    const float* getSmokeTemperatureData() const { return m_T.data(); }
    int getSmokeNx() const { return m_nx; }
    int getSmokeNy() const { return m_ny; }
    int getSmokeNz() const { return m_nz; }
    int getSmokeCellCount() const;

private:
    Grid m_grid;
    std::vector<Obstacle> m_obstacles;
    WallWithHole m_ceiling;
    glm::vec3 m_spawnerPosition{ 0.f, -50.f, 0.f };

    int m_nx{ 0 }, m_ny{ 0 }, m_nz{ 0 };
    float m_ambientPressure{ 1013.25f };
    float m_gravity{ -9.81f };  // Negative = down in Y-up system
    float m_timeScale{ 1.0f };
    float m_Tamb{ 0.f };
    float m_buoyancyAlpha{ 0.02f };
    float m_buoyancyBeta{ 0.8f };
    float m_tempCooling{ 0.1f };
    float m_diffusion{ 0.5f };
    float m_injectRate{ 15.f };
    int m_injectRadius{ 2 };
    bool m_injectCylinder{ true };

    std::vector<float> m_u, m_v, m_w, m_p, m_s, m_T;
    std::vector<float> m_uTmp, m_vTmp, m_wTmp, m_sTmp, m_TTmp, m_pTmp;
    std::vector<uint8_t> m_solid;

    int flidx(int i, int j, int k) const { return i + j * m_nx + k * m_nx * m_ny; }
    glm::vec3 cellCenter(int i, int j, int k) const;
    bool isSolid(int i, int j, int k) const;
    bool isPointInObstacle(const glm::vec3& p) const;
    bool isPointInCeiling(const glm::vec3& p) const;

    void buildSolidMask();
    void allocFluid();
    void runFluid(float dt);
    void injectSmoke(float dt);
    void diffuse(float dt);
    void coolTemperature(float dt);
    void addBuoyancyGravity(float dt);
    void advect(float dt);
    void pressureSolve(int iterations);
    void project();

    float sampleTrilinear(const std::vector<float>& f, float cx, float cy, float cz) const;
};
