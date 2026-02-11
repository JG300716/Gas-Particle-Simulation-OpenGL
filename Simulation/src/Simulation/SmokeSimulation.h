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

class SmokeSimulation {
public:
    SmokeSimulation();
    ~SmokeSimulation();

    bool initialize(const UINT32& simulationWidth = 40, const UINT32& simulationHeight = 40,
                    const UINT32& simulationDepth = 40);

    void run(float deltaTime);

    // Parametry
    #pragma region gettery/settery
    [[nodiscard]] float getGravity() const { return m_gravity; }
    void setGravity(float v) { m_gravity = v; }
    [[nodiscard]] float getTimeScale() const { return m_timeScale; }
    void setTimeScale(float v) { m_timeScale = std::max(0.0f, v); }
    [[nodiscard]] float getTempAmbient() const { return m_Tamb; }
    void setTempAmbient(float v) { m_Tamb = v; }
    [[nodiscard]] float getBuoyancyAlpha() const { return m_buoyancyAlpha; }
    void setBuoyancyAlpha(float v) { m_buoyancyAlpha = v; }
    [[nodiscard]] float getBuoyancyBeta() const { return m_buoyancyBeta; }
    void setBuoyancyBeta(float v) { m_buoyancyBeta = v; }
    [[nodiscard]] float getAirDensity() const { return m_airDensity; }
    void setAirDensity(float v) { m_airDensity = std::max(1.f, v); }
    [[nodiscard]] float getSmokeParticleDensity() const { return m_smokeParticleDensity; }
    void setSmokeParticleDensity(float v) { m_smokeParticleDensity = std::max(1.f, v); }
    [[nodiscard]] float getTempCooling() const { return m_tempCooling; }
    void setTempCooling(float v) { m_tempCooling = std::max(0.f, std::min(10.f, v)); }
    [[nodiscard]] float getDiffusion() const { return m_diffusion; }
    void setDiffusion(float v) { m_diffusion = std::max(0.f, std::min(10.f, v)); }
    [[nodiscard]] float getDissipation() const { return m_dissipation; }
    void setDissipation(float v) { m_dissipation = std::max(0.f, std::min(1.f, v)); }
    [[nodiscard]] float getVelocityDissipation() const { return m_velocityDissipation; }
    void setVelocityDissipation(float v) { m_velocityDissipation = std::max(0.f, std::min(2.f, v)); }
    [[nodiscard]] float getTurbulence() const { return m_turbulence; }
    void setTurbulence(float v) { m_turbulence = std::max(0.f, std::min(5.f, v)); }
    [[nodiscard]] float getSmallScaleTurbulenceGain() const { return m_smallScaleTurbulenceGain; }
    void setSmallScaleTurbulenceGain(float v) { m_smallScaleTurbulenceGain = std::max(0.f, std::min(3.f, v)); }
    [[nodiscard]] float getVorticity() const { return m_vorticity; }
    void setVorticity(float v) { m_vorticity = std::max(0.f, std::min(10.f, v)); }
    [[nodiscard]] float getInjectRate() const { return m_injectRate; }
    void setInjectRate(float v) { m_injectRate = std::max(0.f, std::min(50.f, v)); }
    [[nodiscard]] float getInjectVelocity() const { return m_injectVelocity; }
    void setInjectVelocity(float v) { m_injectVelocity = std::max(0.f, std::min(20.f, v)); }
    [[nodiscard]] UINT8 getJacobiIterations() const { return m_numJacobiIterations; }
    void setJacobiIterations(UINT8 v) { m_numJacobiIterations = v; }
    [[nodiscard]] int getInjectRadius() const { return m_injectRadius; }
    void setInjectRadius(int v) { m_injectRadius = std::max(1, std::min(10, v)); }

    void setGridSize(int sizeX, int sizeY, int sizeZ);
    [[nodiscard]] int getGridSizeX() const { return m_grid.getSizeX(); }
    [[nodiscard]] int getGridSizeY() const { return m_grid.getSizeY(); }
    [[nodiscard]] int getGridSizeZ() const { return m_grid.getSizeZ(); }

    [[nodiscard]] glm::vec3 getSpawnerPosition() const { return m_spawnerPosition; }
    void setSpawnerPosition(const glm::vec3& p) { m_spawnerPosition = p + glm::vec3{0.5f, 0, 0.5f}; }
    #pragma endregion
    
    void addObstacle(const Obstacle& o);
    void updateObstacle(size_t i, const Obstacle& o);
    void removeObstacle(size_t i);
    void clearObstacles();
    [[nodiscard]] const std::vector<Obstacle>& getObstacles() const { return m_obstacles; }

    WallWithHole& getCeiling() { return m_ceiling; }
    [[nodiscard]] const WallWithHole& getCeiling() const { return m_ceiling; }
    void setCeilingEnabled(bool enabled);
    void setCeilingHole(const glm::vec2& center, const glm::vec2& size);
    void updateCeiling();

    [[nodiscard]] const Grid& getGrid() const { return m_grid; }

    void clearSmoke();

    [[nodiscard]] const float* getSmokeDensityData() const { return m_density.data(); }
    [[nodiscard]] const float* getSmokeTemperatureData() const { return m_temperature.data(); }
    [[nodiscard]] int getSmokeNx() const { return m_nx; }
    [[nodiscard]] int getSmokeNy() const { return m_ny; }
    [[nodiscard]] int getSmokeNz() const { return m_nz; }
    [[nodiscard]] int getSmokeCellCount() const;

    static constexpr UINT8 minJacobiIterationsValue = 0;
    static constexpr UINT8 maxJacobiIterationsValue = 100;
private:
    Grid m_grid;
    std::vector<Obstacle> m_obstacles;
    WallWithHole m_ceiling;
    glm::vec3 m_spawnerPosition{ 0.f, -10.f, 0.f };

    int m_nx{ 0 }, m_ny{ 0 }, m_nz{ 0 };
    float m_gravity{ -9.81f };  // Negative = down in Y-up system
    float m_timeScale{ 1.0f };
    float m_Tamb{ 30.f };
    float m_buoyancyAlpha{ 1.0f };    // skala wpływu różnicy gęstości na wypór
    float m_buoyancyBeta{ 1.0f };     // siła wyporu termicznego
    float m_airDensity{ 1200.f };      // gęstość powietrza [g/m³]
    float m_smokeParticleDensity{ 1100.f }; // gęstość cząsteczek dymu [g/m³] (lżejsza = unosi się)
    float m_tempCooling{ 0.1f };       // szybkość chłodzenia
    float m_diffusion{ 0.04f };        // dyfuzja gęstości/temperatury
    float m_dissipation{ 0.005f };      // szybkość zanikania gęstości
    float m_velocityDissipation{ 0.2f }; // opór prędkości (drag) - dym naturalnie zwalnia
    float m_turbulence{ 0.01f };        // siła turbulencji
    float m_smallScaleTurbulenceGain{ 0.0f }; // turbulencja w pustych komórkach (życie zanim dotrze dym)
    float m_vorticity{ 0.3f };         // siła wirów (vorticity confinement)
    float m_injectRate{ 5.f };         // szybkość wstrzykiwania gęstości (ZMNIEJSZONA)
    float m_injectVelocity{ 8.0f };    // początkowa prędkość dymu w górę
    int m_injectRadius{ 1 };

    // Pola płynu (na siatce)
    std::vector<float> m_velocityX, m_velocityY, m_velocityZ;  // prędkość (składowe X,Y,Z)
    std::vector<float> m_pressure;                             // ciśnienie (solver nieściśliwości)
    std::vector<float> m_density;       // stężenie dymu 0–1 (udział w komórce), nie g/m³
    std::vector<float> m_temperature;                           // temperatura
    // Bufory tymczasowe: zapis nowych wartości w krokach (adwekcja, dyfuzja, pressure).
    // Nie da się aktualizować "w miejscu" – potrzebne są stare wartości dla wszystkich komórek.
    std::vector<float> m_tmpVelocityX, m_tmpVelocityY, m_tmpVelocityZ;
    std::vector<float> m_tmpDensity, m_tmpTemperature, m_tmpPressure;
    std::vector<uint8_t> m_solid;

    [[nodiscard]] int flidx(int i, int j, int k) const { return i + j * m_nx + k * m_nx * m_ny; }
    [[nodiscard]] glm::vec3 cellCenter(int i, int j, int k) const;
    [[nodiscard]] bool isSolid(int i, int j, int k) const;
    [[nodiscard]] bool isPointInObstacle(const glm::vec3& p) const;
    [[nodiscard]] bool isPointInCeiling(const glm::vec3& p) const;

    void buildSolidMask();
    void allocFluid();
    void runFluid(float dt);
    void injectSmoke(float dt);
    void diffuse(float dt);
    void coolTemperature(float dt);
    void dissipate(float dt);
    void applyVelocityDissipation(float dt);
    void addBuoyancyGravity(float dt);
    void addSmallScaleTurbulence(float dt);
    void addTurbulence(float dt);
    void addVorticityConfinement(float dt);
    void advect(float dt);
    void pressureSolve(int iterations);
    void project();

    [[nodiscard]] float sampleTrilinear(const std::vector<float>& f, float cx, float cy, float cz) const;
    [[nodiscard]] float noise3D(float x, float y, float z) const;
    [[nodiscard]] glm::vec3 curlNoise3D(float x, float y, float z) const;
    
    float m_time{ 0.f };
    float m_accumulator = 0.f;
    const float FIXED_DT = 1.f / 60.f;  // 60Hz physics tick
    const float MAX_DT = 0.25f;         // Prevent spiral of death

    UINT8 m_numJacobiIterations{ 30 };
};
