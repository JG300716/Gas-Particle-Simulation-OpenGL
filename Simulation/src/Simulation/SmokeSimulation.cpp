#include "SmokeSimulation.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

namespace {
    unsigned s_dbgFrames = 0;
    constexpr unsigned DBG_LOG_INTERVAL = 60;
    glm::ivec3 s_dbgInjectSp(-1, -1, -1);

#if defined(_DEBUG) || defined(DEBUG)
    void dbgLog(const char* tag, const std::string& msg) {
        Logger::logWithTime(tag, msg);
    }
#else
    void dbgLog(const char* tag, const std::string& msg){}
#endif
    
    
}

SmokeSimulation::SmokeSimulation()
    : m_grid(20, 20, 20)
{
    
}

SmokeSimulation::~SmokeSimulation() = default;

bool SmokeSimulation::initialize(const UINT32& simulationWidth, const UINT32& simulationHeight,
                                 const UINT32& simulationDepth) {
    m_nx = static_cast<int>(simulationWidth);
    m_ny = static_cast<int>(simulationHeight);
    m_nz = static_cast<int>(simulationDepth);
    m_grid = Grid(m_nx, m_ny, m_nz);
    allocFluid();

    // Inicjalizacja sufitu z dziurą na środku
    glm::vec3 mn = m_grid.getMinBounds(), mx = m_grid.getMaxBounds();
    m_ceiling.position = glm::vec3(0.f, mx.y - 0.5f, 0.f);
    m_ceiling.width = mx.x - mn.x;
    m_ceiling.depth = mx.z - mn.z;
    m_ceiling.thickness = 1.f;
    m_ceiling.holeCenter = glm::vec2(0.f, 0.f);
    m_ceiling.holeSize = glm::vec2(m_ceiling.width * 0.3f, m_ceiling.depth * 0.3f);
    m_ceiling.enabled = false;

    buildSolidMask();
    std::ostringstream os;
    os << "grid " << m_nx << "x" << m_ny << "x" << m_nz
       << " bounds [" << mn.x << "," << mn.y << "," << mn.z << "]..[" << mx.x << "," << mx.y << "," << mx.z << "]";
    dbgLog("INIT", os.str());
    
    return true;
}

void SmokeSimulation::setGridSize(int sizeX, int sizeY, int sizeZ) {
    m_nx = std::max(1, sizeX);
    m_ny = std::max(1, sizeY);
    m_nz = std::max(1, sizeZ);
    m_grid = Grid(m_nx, m_ny, m_nz);
    allocFluid();

    // Aktualizuj sufit do nowego rozmiaru
    glm::vec3 mn = m_grid.getMinBounds(), mx = m_grid.getMaxBounds();
    m_ceiling.position = glm::vec3(0.f, mx.y - 0.5f, 0.f);
    m_ceiling.width = mx.x - mn.x;
    m_ceiling.depth = mx.z - mn.z;

    buildSolidMask();
}

void SmokeSimulation::allocFluid() {
    const size_t n = static_cast<size_t>(m_nx) * m_ny * m_nz;
    m_velocityX.assign(n, 0.f);
    m_velocityY.assign(n, 0.f);
    m_velocityZ.assign(n, 0.f);
    m_pressure.assign(n, 0.f);
    m_density.assign(n, 0.f);
    m_temperature.assign(n, 0.f);
    m_solid.assign(n, 0);
    m_tmpVelocityX.resize(n);
    m_tmpVelocityY.resize(n);
    m_tmpVelocityZ.resize(n);
    m_tmpDensity.resize(n);
    m_tmpTemperature.resize(n);
    m_tmpPressure.resize(n);
}

void SmokeSimulation::buildSolidMask() {
    for (int k = 0; k < m_nz; ++k)
        for (int j = 0; j < m_ny; ++j)
            for (int i = 0; i < m_nx; ++i) {
                glm::vec3 c = cellCenter(i, j, k);
                m_solid[flidx(i, j, k)] = (isPointInObstacle(c) || isPointInCeiling(c)) ? 1 : 0;
            }
}

bool SmokeSimulation::isPointInObstacle(const glm::vec3& p) const {
    for (const auto& o : m_obstacles) {
        glm::mat3 R = glm::mat3(glm::eulerAngleXYZ(
            glm::radians(o.rotation.x), glm::radians(o.rotation.y), glm::radians(o.rotation.z)));
        glm::vec3 local = glm::inverse(R) * (p - o.position);
        glm::vec3 h = o.size * 0.5f * o.scale;
        if (local.x >= -h.x && local.x <= h.x && local.y >= -h.y && local.y <= h.y && local.z >= -h.z && local.z <= h.z)
            return true;
    }
    return false;
}

bool SmokeSimulation::isPointInCeiling(const glm::vec3& p) const {
    if (!m_ceiling.enabled) return false;
    // Sprawdź czy punkt jest w obszarze ściany (bez dziury)
    float halfW = m_ceiling.width * 0.5f;
    float halfD = m_ceiling.depth * 0.5f;
    float halfT = m_ceiling.thickness * 0.5f;
    // Lokalne współrzędne względem środka sufitu
    float lx = p.x - m_ceiling.position.x;
    float ly = p.y - m_ceiling.position.y;
    float lz = p.z - m_ceiling.position.z;
    // Czy punkt jest w boksie ściany?
    if (lx < -halfW || lx > halfW) return false;
    if (ly < -halfT || ly > halfT) return false;
    if (lz < -halfD || lz > halfD) return false;
    // Punkt jest w boksie - sprawdź czy NIE jest w dziurze
    float hx = m_ceiling.holeSize.x * 0.5f;
    float hz = m_ceiling.holeSize.y * 0.5f;
    float dx = lx - m_ceiling.holeCenter.x;
    float dz = lz - m_ceiling.holeCenter.y;
    if (dx >= -hx && dx <= hx && dz >= -hz && dz <= hz) {
        return false; // W dziurze - nie jest solid
    }
    return true; // W ścianie (nie w dziurze) - jest solid
}

void SmokeSimulation::setCeilingEnabled(bool enabled) {
    m_ceiling.enabled = enabled;
    buildSolidMask();
}

void SmokeSimulation::setCeilingHole(const glm::vec2& center, const glm::vec2& size) {
    m_ceiling.holeCenter = center;
    m_ceiling.holeSize = size;
    buildSolidMask();
}

void SmokeSimulation::updateCeiling() {
    buildSolidMask();
}

bool SmokeSimulation::isSolid(int i, int j, int k) const {
    if (i < 0 || i >= m_nx || j < 0 || j >= m_ny || k < 0 || k >= m_nz) return true;
    return m_solid[flidx(i, j, k)] != 0;
}

glm::vec3 SmokeSimulation::cellCenter(int i, int j, int k) const {
    return m_grid.gridToWorld(glm::ivec3(i, j, k));
}

float SmokeSimulation::sampleTrilinear(const std::vector<float>& f, float cx, float cy, float cz) const {
    cx = std::max(0.f, std::min(static_cast<float>(m_nx) - 1.001f, cx));
    cy = std::max(0.f, std::min(static_cast<float>(m_ny) - 1.001f, cy));
    cz = std::max(0.f, std::min(static_cast<float>(m_nz) - 1.001f, cz));
    
    const int i0 = static_cast<int>(std::floor(cx));
    const int j0 = static_cast<int>(std::floor(cy));
    const int k0 = static_cast<int>(std::floor(cz));
    
    const int i1 = std::min(i0 + 1, m_nx - 1);
    const int j1 = std::min(j0 + 1, m_ny - 1);
    const int k1 = std::min(k0 + 1, m_nz - 1);
    
    const float fx = cx - i0;
    const float fy = cy - j0;
    const float fz = cz - k0;
    
    const float v000 = f[flidx(i0, j0, k0)];
    const float v100 = f[flidx(i1, j0, k0)];
    const float v010 = f[flidx(i0, j1, k0)];
    const float v110 = f[flidx(i1, j1, k0)];
    const float v001 = f[flidx(i0, j0, k1)];
    const float v101 = f[flidx(i1, j0, k1)];
    const float v011 = f[flidx(i0, j1, k1)];
    const float v111 = f[flidx(i1, j1, k1)];
    
    const float v00 = v000 + fx * (v100 - v000);
    const float v10 = v010 + fx * (v110 - v010);
    const float v01 = v001 + fx * (v101 - v001);
    const float v11 = v011 + fx * (v111 - v011);
    
    const float v0 = v00 + fy * (v10 - v00);
    const float v1 = v01 + fy * (v11 - v01);
    
    return v0 + fz * (v1 - v0);
}

void SmokeSimulation::run(float deltaTime) {
    ++s_dbgFrames;

    deltaTime = std::min(deltaTime, MAX_DT);
    m_accumulator += deltaTime * m_timeScale;

    while (m_accumulator >= FIXED_DT) {
        runFluid(FIXED_DT);
        m_accumulator -= FIXED_DT;
    }
}

void SmokeSimulation::runFluid(float dt) {
    const bool dbg = (s_dbgFrames % DBG_LOG_INTERVAL == 1);
    m_time += dt;

    addSmallScaleTurbulence(dt);
    injectSmoke(dt);
    addBuoyancyGravity(dt);
    
    addTurbulence(dt);
    addVorticityConfinement(dt);

    advect(dt);
    diffuse(dt);

    pressureSolve(m_numJacobiIterations);
    project();

    applyVelocityDissipation(dt);
    coolTemperature(dt);
    dissipate(dt);

    if (dbg) {
        constexpr float thresh = 0.01f;
        int imin = m_nx, imax = -1, jmin = m_ny, jmax = -1, kmin = m_nz, kmax = -1;
        int n = 0;
        for (int k = 0; k < m_nz; ++k)
            for (int j = 0; j < m_ny; ++j)
                for (int i = 0; i < m_nx; ++i) {
                    if (m_density[flidx(i, j, k)] <= thresh) continue;
                    ++n;
                    imin = std::min(imin, i); imax = std::max(imax, i);
                    jmin = std::min(jmin, j); jmax = std::max(jmax, j);
                    kmin = std::min(kmin, k); kmax = std::max(kmax, k);
                }
        glm::vec3 mn = m_grid.getMinBounds(), mx = m_grid.getMaxBounds();
        std::ostringstream os;
        os << "frame=" << s_dbgFrames << " cells=" << n;
        if (n > 0)
            os << " span i[" << imin << ".." << imax << "] j[" << jmin << ".." << jmax << "] k[" << kmin << ".." << kmax << "]";
        else
            os << " span N/A";
        dbgLog("BOUNDS", os.str());
        if (n > 0) {
            glm::vec3 pmin = cellCenter(imin, jmin, kmin);
            glm::vec3 pmax = cellCenter(imax, jmax, kmax);
            std::ostringstream os2;
            os2 << "smoke world [" << pmin.x << "," << pmin.y << "," << pmin.z << "]..[" << pmax.x << "," << pmax.y << "," << pmax.z
                << "] grid [" << mn.x << "," << mn.y << "," << mn.z << "]..[" << mx.x << "," << mx.y << "," << mx.z << "]";
            dbgLog("BOUNDS", os2.str());
        }
    }
}

void SmokeSimulation::injectSmoke(float dt) {
    glm::vec3 mn = m_grid.getMinBounds();
    glm::vec3 mx = m_grid.getMaxBounds();
    glm::vec3 injectCenterRaw = m_spawnerPosition + glm::vec3(0.f, 1.f, 0.f);
    glm::vec3 injectCenter = injectCenterRaw;
    injectCenter.x = std::max(mn.x, std::min(mx.x, injectCenter.x));
    injectCenter.y = std::max(mn.y, std::min(mx.y, injectCenter.y));
    injectCenter.z = std::max(mn.z, std::min(mx.z, injectCenter.z));
    glm::ivec3 sp = m_grid.worldToGrid(injectCenter);
    sp.x = std::max(0, std::min(m_nx - 1, sp.x));
    sp.y = std::max(0, std::min(m_ny - 1, sp.y));
    sp.z = std::max(0, std::min(m_nz - 1, sp.z));
    s_dbgInjectSp = sp;
    
    const float densityRate = m_injectRate * dt;      // ile gęstości dodać na klatkę
    const float T_inj = m_Tamb + 300.f;               // temperatura gorącego dymu (zwiększona)
    const float R2 = static_cast<float>(m_injectRadius * m_injectRadius);

    const bool dbg = (s_dbgFrames % DBG_LOG_INTERVAL == 1);
    int injected = 0;
    float sBefore = 0.f;
    size_t idSp = flidx(sp.x, sp.y, sp.z);
    if (dbg) sBefore = m_density[idSp];

    for (int di = -m_injectRadius; di <= m_injectRadius; ++di)
        for (int dj = 0; dj <= m_injectRadius; ++dj)
            for (int dk = -m_injectRadius; dk <= m_injectRadius; ++dk) {
                int i = sp.x + di, j = sp.y + dj, k = sp.z + dk;
                if (i < 0 || i >= m_nx || j < 0 || j >= m_ny || k < 0 || k >= m_nz) continue;
                if (isSolid(i, j, k)) continue;
                
                float fdx = static_cast<float>(i - sp.x), fdz = static_cast<float>(k - sp.z);
                float fdy = static_cast<float>(j - sp.y);
                if (fdx * fdx + fdy * fdy + fdz * fdz > R2) continue;
                
                size_t id = flidx(i, j, k);
                float oldDensity = m_density[id];
                
                // Dodaj gęstość (nasycenie do 1.0)
                float newDensity = std::min(1.f, oldDensity + densityRate);
                m_density[id] = newDensity;
                
                m_temperature[id] = T_inj;
                
                if (m_velocityY[id] < m_injectVelocity) {
                    float targetV = m_injectVelocity;
                    m_velocityY[id] = m_velocityY[id] + (targetV - m_velocityY[id]) * std::min(1.0f, dt * 10.0f);
                }
                
                ++injected;
            }

    if (dbg) {
        std::ostringstream os;
        os << "frame=" << s_dbgFrames << " spawner=(" << m_spawnerPosition.x << "," << m_spawnerPosition.y << "," << m_spawnerPosition.z << ")"
           << " injectRate=" << m_injectRate << " injectVel=" << m_injectVelocity
           << " dt=" << dt << " injected=" << injected
           << " s@sp " << sBefore << "->" << m_density[idSp];
        dbgLog("SPAWNER", os.str());
    }
}

void SmokeSimulation::clearSmoke() {
    std::ranges::fill(m_density.begin(), m_density.end(), 0.f);
    std::ranges::fill(m_temperature.begin(), m_temperature.end(), m_Tamb);
    std::ranges::fill(m_velocityX.begin(), m_velocityX.end(), 0.f);
    std::ranges::fill(m_velocityY.begin(), m_velocityY.end(), 0.f);
    std::ranges::fill(m_velocityZ.begin(), m_velocityZ.end(), 0.f);
    std::ranges::fill(m_pressure.begin(), m_pressure.end(), 0.f);
}

void SmokeSimulation::diffuse(float dt) {
    const float dx = 1.f;
    const float nu = m_diffusion;
    const float diffCoeff = nu * dt / (dx * dx);
    const int iters = 12;

    auto isGas = [this](int i, int j, int k) {
        return i >= 0 && i < m_nx && j >= 0 && j < m_ny && k >= 0 && k < m_nz
            && !isSolid(i, j, k);
    };

    for (int it = 0; it < iters; ++it) {
        for (int k = 0; k < m_nz; ++k)
            for (int j = 0; j < m_ny; ++j)
                for (int i = 0; i < m_nx; ++i) {
                    if (isSolid(i, j, k) ) continue;
                    size_t id = flidx(i, j, k);
                    float sumS = 0.f, sumT = 0.f;
                    int nn = 0;
                    if (isGas(i + 1, j, k)) { sumS += m_density[flidx(i + 1, j, k)]; sumT += m_temperature[flidx(i + 1, j, k)]; ++nn; }
                    if (isGas(i - 1, j, k)) { sumS += m_density[flidx(i - 1, j, k)]; sumT += m_temperature[flidx(i - 1, j, k)]; ++nn; }
                    if (isGas(i, j + 1, k)) { sumS += m_density[flidx(i, j + 1, k)]; sumT += m_temperature[flidx(i, j + 1, k)]; ++nn; }
                    if (isGas(i, j - 1, k)) { sumS += m_density[flidx(i, j - 1, k)]; sumT += m_temperature[flidx(i, j - 1, k)]; ++nn; }
                    if (isGas(i, j, k + 1)) { sumS += m_density[flidx(i, j, k + 1)]; sumT += m_temperature[flidx(i, j, k + 1)]; ++nn; }
                    if (isGas(i, j, k - 1)) { sumS += m_density[flidx(i, j, k - 1)]; sumT += m_temperature[flidx(i, j, k - 1)]; ++nn; }
                    nn = std::max(1, nn);
                    float denom = 1.f + static_cast<float>(nn) * diffCoeff;
                    m_tmpDensity[id] = (m_density[id] + diffCoeff * sumS) / denom;
                    m_tmpTemperature[id] = (m_temperature[id] + diffCoeff * sumT) / denom;
                }
        m_density.swap(m_tmpDensity);
        m_temperature.swap(m_tmpTemperature);
    }
}

void SmokeSimulation::coolTemperature(float dt) {
    const float d = std::max(0.f, 1.f - m_tempCooling * dt);
    for (int k = 0; k < m_nz; ++k)
        for (int j = 0; j < m_ny; ++j)
            for (int i = 0; i < m_nx; ++i) {
                if (isSolid(i, j, k)) continue;
                size_t id = flidx(i, j, k);
                m_temperature[id] = m_Tamb + (m_temperature[id] - m_Tamb) * d;
            }
}

float SmokeSimulation::noise3D(float x, float y, float z) const {
    auto hash = [](int n) -> float {
        n = (n << 13) ^ n;
        return 1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f;
    };
    
    const int ix = static_cast<int>(std::floor(x));
    const int iy = static_cast<int>(std::floor(y));
    const int iz = static_cast<int>(std::floor(z));
    float fx = x - ix;
    float fy = y - iy;
    float fz = z - iz;
    
    // Smooth interpolation
    fx = fx * fx * (3.0f - 2.0f * fx);
    fy = fy * fy * (3.0f - 2.0f * fy);
    fz = fz * fz * (3.0f - 2.0f * fz);
    
    auto idx = [](int x, int y, int z) { return x + y * 57 + z * 113; };
    
    const float v000 = hash(idx(ix, iy, iz));
    const float v100 = hash(idx(ix + 1, iy, iz));
    const float v010 = hash(idx(ix, iy + 1, iz));
    const float v110 = hash(idx(ix + 1, iy + 1, iz));
    const float v001 = hash(idx(ix, iy, iz + 1));
    const float v101 = hash(idx(ix + 1, iy, iz + 1));
    const float v011 = hash(idx(ix, iy + 1, iz + 1));
    const float v111 = hash(idx(ix + 1, iy + 1, iz + 1));
    
    const float v00 = v000 * (1 - fx) + v100 * fx;
    const float v01 = v001 * (1 - fx) + v101 * fx;
    const float v10 = v010 * (1 - fx) + v110 * fx;
    const float v11 = v011 * (1 - fx) + v111 * fx;
    
    const float v0 = v00 * (1 - fy) + v10 * fy;
    const float v1 = v01 * (1 - fy) + v11 * fy;
    
    return v0 * (1 - fz) + v1 * fz;
}

glm::vec3 SmokeSimulation::curlNoise3D(float x, float y, float z) const {
    const float e = 0.0001f;
    float inv2e = 0.5f / e;
    
    // Use 3 offset noise samples as vector potential components
    auto potential = [this](float x, float y, float z) -> glm::vec3 {
        return glm::vec3(
            noise3D(x, y, z),
            noise3D(x + 31.337f, y, z),      // Offset to decorrelate
            noise3D(x, y + 59.432f, z)
        );
    };
    
    glm::vec3 px1 = potential(x + e, y, z);
    glm::vec3 px0 = potential(x - e, y, z);
    glm::vec3 py1 = potential(x, y + e, z);
    glm::vec3 py0 = potential(x, y - e, z);
    glm::vec3 pz1 = potential(x, y, z + e);
    glm::vec3 pz0 = potential(x, y, z - e);
    
    // curl(ψ) = (∂ψ_z/∂y - ∂ψ_y/∂z, ∂ψ_x/∂z - ∂ψ_z/∂x, ∂ψ_y/∂x - ∂ψ_x/∂y)
    return glm::vec3(
        (pz1.z - pz0.z) * inv2e - (py1.y - py0.y) * inv2e,
        (px1.x - px0.x) * inv2e - (pz1.z - pz0.z) * inv2e,
        (py1.y - py0.y) * inv2e - (px1.x - px0.x) * inv2e
    );
}

// Small Scale Turbulence - tylko w PUSTYCH komórkach (density < threshold).
// Gdy dym dotrze do tych obszarów, pole prędkości ma już "życie" - naturalny ruch.
void SmokeSimulation::addSmallScaleTurbulence(float dt) {
    if (m_smallScaleTurbulenceGain <= 0.f) return;
    
    constexpr float emptyThreshold = 0.01f;  // komórka uznana za pustą
    const float cellScale = 0.4f;             // skala szumu na rozmiar komórki
    const float timeScale = 0.2f;
    
    for (int k = 1; k < m_nz - 1; ++k)
        for (int j = 1; j < m_ny - 1; ++j)
            for (int i = 1; i < m_nx - 1; ++i) {
                if (isSolid(i, j, k)) continue;
                size_t id = flidx(i, j, k);
                
                if (m_density[id] > emptyThreshold) continue;  // tylko puste komórki
                
                float nx = i * cellScale;
                float ny = j * cellScale;
                float nz = k * cellScale;
                float nt = m_time * timeScale;
                
                float nU = noise3D(nx + nt, ny + 50.f, nz);
                float nV = noise3D(nx + 100.f, ny + nt, nz);
                float nW = noise3D(nx, ny + 150.f, nz + nt);
                
                float strength = m_smallScaleTurbulenceGain * dt;
                
                m_velocityX[id] += nU * strength;
                m_velocityY[id] += nV * strength * 0.2f;  // mniej w pionie
                m_velocityZ[id] += nW * strength;
            }
}

// Dodaje turbulencję (szum) do pola prędkości - dla chaotycznego ruchu
void SmokeSimulation::addTurbulence(float dt) {
    if (m_turbulence <= 0.f) return;
    
    const float noiseScale = 0.12f;  // skala szumu w przestrzeni
    const float timeScale = 0.3f;    // jak szybko szum się zmienia (wolniej = płynniejszy ruch)
    
    for (int k = 0; k < m_nz; ++k)
        for (int j = 0; j < m_ny; ++j)
            for (int i = 0; i < m_nx; ++i) {
                if (isSolid(i, j, k)) continue;
                size_t id = flidx(i, j, k);
                
                // Turbulencja działa tylko gdzie jest dym
                float density = m_density[id];
                if (density < 0.02f) continue;
                
                float nx = i * noiseScale;
                float ny = j * noiseScale;
                float nz = k * noiseScale;
                float nt = m_time * timeScale;
                
                float strength = m_turbulence * std::sqrt(density) * dt;
                
                glm::vec3 curl = curlNoise3D(nx + nt, ny, nz);  // Lepsze niż curlNoise
                m_velocityX[id] += curl.x * strength;
                m_velocityY[id] += curl.y * strength * 0.5f;
                m_velocityZ[id] += curl.z * strength;
            }
}

// Vorticity confinement - wzmacnia wiry w płynie (realistyczne kłęby dymu)
void SmokeSimulation::addVorticityConfinement(float dt) {
    if (m_vorticity <= 0.f) return;
    
    // Oblicz vorticity (curl prędkości) w każdej komórce
    std::vector<glm::vec3> omega(m_nx * m_ny * m_nz, glm::vec3(0.f));
    
    for (int k = 1; k < m_nz - 1; ++k)
        for (int j = 1; j < m_ny - 1; ++j)
            for (int i = 1; i < m_nx - 1; ++i) {
                if (isSolid(i, j, k)) continue;
                
                // Gradienty prędkości (centralne różnice)
                float dwdy = (m_velocityZ[flidx(i, j + 1, k)] - m_velocityZ[flidx(i, j - 1, k)]) * 0.5f;
                float dvdz = (m_velocityY[flidx(i, j, k + 1)] - m_velocityY[flidx(i, j, k - 1)]) * 0.5f;
                float dudz = (m_velocityX[flidx(i, j, k + 1)] - m_velocityX[flidx(i, j, k - 1)]) * 0.5f;
                float dwdx = (m_velocityZ[flidx(i + 1, j, k)] - m_velocityZ[flidx(i - 1, j, k)]) * 0.5f;
                float dvdx = (m_velocityY[flidx(i + 1, j, k)] - m_velocityY[flidx(i - 1, j, k)]) * 0.5f;
                float dudy = (m_velocityX[flidx(i, j + 1, k)] - m_velocityX[flidx(i, j - 1, k)]) * 0.5f;
                
                // curl = (dw/dy - dv/dz, du/dz - dw/dx, dv/dx - du/dy)
                omega[flidx(i, j, k)] = glm::vec3(dwdy - dvdz, dudz - dwdx, dvdx - dudy);
            }
    
    // Zastosuj siłę vorticity confinement
    for (int k = 1; k < m_nz - 1; ++k)
        for (int j = 1; j < m_ny - 1; ++j)
            for (int i = 1; i < m_nx - 1; ++i) {
                if (isSolid(i, j, k)) continue;
                size_t id = flidx(i, j, k);
                
                // Tylko gdzie jest dym
                if (m_density[id] < 0.01f) continue;
                
                glm::vec3 w = omega[id];
                float wLen = glm::length(w);
                if (wLen < 1e-6f) continue;
                
                // Gradient wielkości vorticity
                float omegaXp = glm::length(omega[flidx(i + 1, j, k)]);
                float omegaXm = glm::length(omega[flidx(i - 1, j, k)]);
                float omegaYp = glm::length(omega[flidx(i, j + 1, k)]);
                float omegaYm = glm::length(omega[flidx(i, j - 1, k)]);
                float omegaZp = glm::length(omega[flidx(i, j, k + 1)]);
                float omegaZm = glm::length(omega[flidx(i, j, k - 1)]);
                
                glm::vec3 eta(omegaXp - omegaXm, omegaYp - omegaYm, omegaZp - omegaZm);
                float etaLen = glm::length(eta);
                if (etaLen < 1e-6f) continue;
                
                glm::vec3 N = eta / etaLen;
                
                // Siła vorticity = epsilon * (N x omega)
                glm::vec3 force = m_vorticity * glm::cross(N, w) * dt;
                
                // Redukuj siły poziome żeby dym szedł głównie w górę
                m_velocityX[id] += force.x;
                m_velocityY[id] += force.y;
                m_velocityZ[id] += force.z;
            }
}

// Dysypacja - powolne zanikanie gęstości dymu
// Wolniejsze przy suficie (górnej granicy) dla akumulacji
void SmokeSimulation::dissipate(float dt) {
    if (m_dissipation <= 0.f) return;
    
    const float topY = static_cast<float>(m_ny - 1);
    
    for (int k = 0; k < m_nz; ++k)
        for (int j = 0; j < m_ny; ++j)
            for (int i = 0; i < m_nx; ++i) {
                if (isSolid(i, j, k)) continue;
                size_t id = flidx(i, j, k);
                
                if (m_density[id] < 0.001f) {
                    m_density[id] = 0.f;
                    continue;
                }
                
                // Współczynnik dysypacji - mniejszy przy suficie dla akumulacji
                float heightRatio = static_cast<float>(j) / topY;
                
                // Przy suficie dysypacja jest wolniejsza (dym gromadzi się pod sufitem)
                float dissipFactor;
                if (heightRatio > 0.85f) {
                    // Wolna dysypacja przy suficie - dym się gromadzi
                    dissipFactor = m_dissipation * 0.3f;
                } else if (heightRatio > 0.7f) {
                    // Stopniowe przejście
                    float t = (heightRatio - 0.7f) / 0.15f;
                    dissipFactor = m_dissipation * (1.0f - t * 0.7f);
                } else {
                    dissipFactor = m_dissipation;
                }
                
                // Eksponencjalne zanikanie
                m_density[id] *= std::max(0.f, 1.f - dissipFactor * dt);
            }
}

void SmokeSimulation::applyVelocityDissipation(float dt) {
    if (m_velocityDissipation <= 0.f) return;
    
    const float decay = std::max(0.f, 1.f - m_velocityDissipation * dt);
    
    for (int k = 0; k < m_nz; ++k)
        for (int j = 0; j < m_ny; ++j)
            for (int i = 0; i < m_nx; ++i) {
                if (isSolid(i, j, k)) continue;
                size_t id = flidx(i, j, k);
                m_velocityX[id] *= decay;
                m_velocityY[id] *= decay;
                m_velocityZ[id] *= decay;
            }
}

void SmokeSimulation::addBuoyancyGravity(float dt) {
    // Wypór od różnicy gęstości (g/m³): (rho_air - rho_smoke)/rho_air * stężenie; + wypór termiczny
    const float rhoRatio = (m_airDensity - m_smokeParticleDensity) / std::max(1.f, m_airDensity);
    for (int k = 0; k < m_nz; ++k)
        for (int j = 0; j < m_ny; ++j)
            for (int i = 0; i < m_nx; ++i) {
                if (isSolid(i, j, k)) continue;
                const size_t id = flidx(i, j, k);
                const float concentration = m_density[id];  // 0–1
                const float buoy = m_buoyancyAlpha * rhoRatio * concentration
                    + m_buoyancyBeta * (m_temperature[id] - m_Tamb);
                m_velocityY[id] += (m_gravity + buoy) * dt;
            }
}

void SmokeSimulation::advect(float dt) {
    const float dx = 1.f;
    const glm::vec3 o = m_grid.getMinBounds();
    m_tmpVelocityX = m_velocityX;
    m_tmpVelocityY = m_velocityY;
    m_tmpVelocityZ = m_velocityZ;
    m_tmpDensity = m_density;
    m_tmpTemperature = m_temperature;

    for (int k = 0; k < m_nz; ++k)
        for (int j = 0; j < m_ny; ++j)
            for (int i = 0; i < m_nx; ++i) {
                if (isSolid(i, j, k)) continue;
                
                size_t id = flidx(i, j, k);
                glm::vec3 pos = cellCenter(i, j, k);
                float u0 = m_velocityX[id], v0 = m_velocityY[id], w0 = m_velocityZ[id];
                
                // Semi-Lagrangian backtrace
                glm::vec3 back = pos - dt * glm::vec3(u0, v0, w0);
                // Cell-centered fields: convert world -> grid index so that cell centers map to integer coords
                glm::vec3 local = (back - o) / dx - glm::vec3(0.5f);
                
                // Sample from previous position (trilinear handles clamping)
                m_tmpVelocityX[id] = sampleTrilinear(m_velocityX, local.x, local.y, local.z);
                m_tmpVelocityY[id] = sampleTrilinear(m_velocityY, local.x, local.y, local.z);
                m_tmpVelocityZ[id] = sampleTrilinear(m_velocityZ, local.x, local.y, local.z);
                m_tmpDensity[id] = sampleTrilinear(m_density, local.x, local.y, local.z);
                m_tmpTemperature[id] = sampleTrilinear(m_temperature, local.x, local.y, local.z);
            }

    m_velocityX.swap(m_tmpVelocityX);
    m_velocityY.swap(m_tmpVelocityY);
    m_velocityZ.swap(m_tmpVelocityZ);
    m_density.swap(m_tmpDensity);
    m_temperature.swap(m_tmpTemperature);
    
    // Boundary conditions - zero velocity through walls
    for (int k = 0; k < m_nz; ++k)
        for (int j = 0; j < m_ny; ++j)
            for (int i = 0; i < m_nx; ++i) {
                size_t id = flidx(i, j, k);
                if (isSolid(i, j, k)) {
                    m_velocityX[id] = m_velocityY[id] = m_velocityZ[id] = 0.f;
                    continue;
                }
                if (i == 0 || i == m_nx - 1) m_velocityX[id] = 0.f;
                if (j == 0 || j == m_ny - 1) m_velocityY[id] = 0.f;  // Floor only
                if (k == 0 || k == m_nz - 1) m_velocityZ[id] = 0.f;
            }
}

void SmokeSimulation::pressureSolve(int iterations) {
    const float dx = 1.f;
    const size_t n = static_cast<size_t>(m_nx) * m_ny * m_nz;
    std::vector<float> div(n, 0.f);

    for (int k = 0; k < m_nz; ++k)
        for (int j = 0; j < m_ny; ++j)
            for (int i = 0; i < m_nx; ++i) {
                if (isSolid(i, j, k) ) continue;
                size_t id = flidx(i, j, k);
                float uR = (i + 1 < m_nx) ? m_velocityX[flidx(i + 1, j, k)] : m_velocityX[id];
                float uL = (i - 1 >= 0) ? m_velocityX[flidx(i - 1, j, k)] : m_velocityX[id];
                float vT = (j + 1 < m_ny) ? m_velocityY[flidx(i, j + 1, k)] : m_velocityY[id];
                float vB = (j - 1 >= 0) ? m_velocityY[flidx(i, j - 1, k)] : m_velocityY[id];
                float wF = (k + 1 < m_nz) ? m_velocityZ[flidx(i, j, k + 1)] : m_velocityZ[id];
                float wK = (k - 1 >= 0) ? m_velocityZ[flidx(i, j, k - 1)] : m_velocityZ[id];
                if (isSolid(i + 1, j, k)) uR = m_velocityX[id];
                if (isSolid(i - 1, j, k)) uL = m_velocityX[id];
                if (isSolid(i, j + 1, k)) vT = m_velocityY[id];
                if (isSolid(i, j - 1, k)) vB = m_velocityY[id];
                if (isSolid(i, j, k + 1)) wF = m_velocityZ[id];
                if (isSolid(i, j, k - 1)) wK = m_velocityZ[id];
                div[id] = ((uR - uL) + (vT - vB) + (wF - wK)) / dx;
            }

    std::ranges::fill(m_pressure.begin(), m_pressure.end(), 0.f);
    const float dx2 = dx * dx;
    auto pNeighbor = [this](int i, int j, int k) {
        if (i < 0 || i >= m_nx || j < 0 || j >= m_ny || k < 0 || k >= m_nz) return 0.f;
        if (isSolid(i, j, k) ) return 0.f;
        return m_pressure[flidx(i, j, k)];
    };

    constexpr int nn = 6;
    
    for (int it = 0; it < iterations; ++it) {
        for (int k = 0; k < m_nz; ++k)
            for (int j = 0; j < m_ny; ++j)
                for (int i = 0; i < m_nx; ++i) {
                    if (isSolid(i, j, k) ) continue;
                    const float sum = pNeighbor(i + 1, j, k) + pNeighbor(i - 1, j, k)
                              + pNeighbor(i, j + 1, k) + pNeighbor(i, j - 1, k)
                              + pNeighbor(i, j, k + 1) + pNeighbor(i, j, k - 1);
                    const size_t id = flidx(i, j, k);
                    m_tmpPressure[id] = (sum - dx2 * div[id]) / static_cast<float>(nn);
                }
        m_pressure.swap(m_tmpPressure);
    }
}

void SmokeSimulation::project() {
    const float dx = 1.f;
    auto pAt = [this](int i, int j, int k) {
        if (i < 0 || i >= m_nx || j < 0 || j >= m_ny || k < 0 || k >= m_nz) return 0.f;
        if (isSolid(i, j, k) ) return 0.f;
        return m_pressure[flidx(i, j, k)];
    };
    
    for (int k = 0; k < m_nz; ++k)
        for (int j = 0; j < m_ny; ++j)
            for (int i = 0; i < m_nx; ++i) {
                if (isSolid(i, j, k) ) continue;
                const size_t id = flidx(i, j, k);
                
                const float pL = pAt(i - 1, j, k), pR = pAt(i + 1, j, k);
                const float pB = pAt(i, j - 1, k), pT = pAt(i, j + 1, k);
                const float pK = pAt(i, j, k - 1), pF = pAt(i, j, k + 1);
                const float dpdx = (pR - pL) / (2.f * dx);
                const float dpdy = (pT - pB) / (2.f * dx);
                const float dpdz = (pF - pK) / (2.f * dx);
                m_velocityX[id] -= dpdx;
                m_velocityY[id] -= dpdy;
                m_velocityZ[id] -= dpdz;
            }
    
    for (int k = 0; k < m_nz; ++k)
        for (int j = 0; j < m_ny; ++j)
            for (int i = 0; i < m_nx; ++i) {
                if (isSolid(i, j, k)) {
                    const size_t id = flidx(i, j, k);
                    m_velocityX[id] = m_velocityY[id] = m_velocityZ[id] = 0.f;
                }
                if (i == 0 || i == m_nx - 1) m_velocityX[flidx(i, j, k)] = 0.f;
                if (j == 0 || j == m_ny - 1) m_velocityY[flidx(i, j, k)] = 0.f;
                if (k == 0 || k == m_nz - 1) m_velocityZ[flidx(i, j, k)] = 0.f;
            }
}

void SmokeSimulation::addObstacle(const Obstacle& o) {
    m_obstacles.push_back(o);
    buildSolidMask();
}

void SmokeSimulation::updateObstacle(size_t i, const Obstacle& o) {
    if (i < m_obstacles.size()) {
        m_obstacles[i] = o;
        buildSolidMask();
    }
}

void SmokeSimulation::removeObstacle(size_t i) {
    if (i < m_obstacles.size()) {
        m_obstacles.erase(m_obstacles.begin() + static_cast<std::ptrdiff_t>(i));
        buildSolidMask();
    }
}

void SmokeSimulation::clearObstacles() {
    m_obstacles.clear();
    buildSolidMask();
}

int SmokeSimulation::getSmokeCellCount() const {
    constexpr float thresh = 0.01f;
    int n = 0;
    for (float v : m_density)
        if (v > thresh) ++n;
    return n;
}
