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

    void dbgLog(const char* tag, const std::string& msg) {
        Logger::logWithTime(tag, msg);
    }
}

SmokeSimulation::SmokeSimulation()
    : m_grid(40, 40, 40),
      m_spawnerPosition(0.f, 0.f, 0.f) {}

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
    m_ceiling.enabled = true;

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
    m_u.assign(n, 0.f);
    m_v.assign(n, 0.f);
    m_w.assign(n, 0.f);
    m_p.assign(n, 0.f);
    m_s.assign(n, 0.f);
    m_T.assign(n, 0.f);
    m_solid.assign(n, 0);
    m_uTmp.resize(n);
    m_vTmp.resize(n);
    m_wTmp.resize(n);
    m_sTmp.resize(n);
    m_TTmp.resize(n);
    m_pTmp.resize(n);
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
    int i0 = static_cast<int>(std::floor(cx)), i1 = std::min(i0 + 1, m_nx - 1);
    int j0 = static_cast<int>(std::floor(cy)), j1 = std::min(j0 + 1, m_ny - 1);
    int k0 = static_cast<int>(std::floor(cz)), k1 = std::min(k0 + 1, m_nz - 1);
    float fx = cx - i0, fy = cy - j0, fz = cz - k0;
    auto v = [&](int i, int j, int k) { return f[flidx(i, j, k)]; };
    float v000 = v(i0, j0, k0), v100 = v(i1, j0, k0), v010 = v(i0, j1, k0), v110 = v(i1, j1, k0);
    float v001 = v(i0, j0, k1), v101 = v(i1, j0, k1), v011 = v(i0, j1, k1), v111 = v(i1, j1, k1);
    float a = v000 * (1 - fx) + v100 * fx;
    float b = v010 * (1 - fx) + v110 * fx;
    float c = v001 * (1 - fx) + v101 * fx;
    float d = v011 * (1 - fx) + v111 * fx;
    float e = a * (1 - fy) + b * fy;
    float g = c * (1 - fy) + d * fy;
    return e * (1 - fz) + g * fz;
}

void SmokeSimulation::run(float deltaTime) {
    ++s_dbgFrames;
    float dt = deltaTime * m_timeScale;
    if (m_timeScale <= 0.f || m_nx * m_ny * m_nz == 0) return;
    runFluid(dt);
}

void SmokeSimulation::runFluid(float dt) {
    const bool dbg = (s_dbgFrames % DBG_LOG_INTERVAL == 1);

    injectSmoke(dt);
    addBuoyancyGravity(dt);

    if (dbg && s_dbgInjectSp.x >= 0 && s_dbgInjectSp.y >= 0 && s_dbgInjectSp.z >= 0) {
        int i = s_dbgInjectSp.x, j = s_dbgInjectSp.y, k = s_dbgInjectSp.z;
        if (i < m_nx && j < m_ny && k < m_nz) {
            size_t id = flidx(i, j, k);
            glm::vec3 c = cellCenter(i, j, k);
            std::ostringstream os;
            os << "frame=" << s_dbgFrames << " gravity=" << m_gravity << " +Y=up | u,v,w@sp=(" << m_u[id] << "," << m_v[id] << "," << m_w[id]
               << ") world=(" << c.x << "," << c.y << "," << c.z << ")";
            dbgLog("DIRECTION", os.str());
        }
    }

    advect(dt);
    diffuse(dt);
    pressureSolve(40);
    project();
    coolTemperature(dt);

    if (dbg) {
        constexpr float thresh = 0.01f;
        int imin = m_nx, imax = -1, jmin = m_ny, jmax = -1, kmin = m_nz, kmax = -1;
        int n = 0;
        for (int k = 0; k < m_nz; ++k)
            for (int j = 0; j < m_ny; ++j)
                for (int i = 0; i < m_nx; ++i) {
                    if (m_s[flidx(i, j, k)] <= thresh) continue;
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

// === GENEROWANIE CZĄSTECZEK DYMU I NADAWANIE PRĘDKOŚCI ===
// Miejsce: SmokeSimulation::injectSmoke() poniżej.
// - Środek wstrzykiwania: spawner + (0,1,0), przeliczany na indeks siatki sp.
// - Pętla po komórkach w cylindrze/kuli wokół sp: dla każdej komórki (i,j,k)
//   DENSITY i TEMPERATURE: m_s[id] += rate, m_T[id] = T_inj
//   PRĘDKOŚĆ (głównie Y):  m_v[id] += vUp * rate (główna siła w górę)
//   + małe perturbacje w X i Z dla naturalnego falowania dymu
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
    const float rate = m_injectRate * dt;
    const float T_inj = m_Tamb + 150.f;
    const float vUp = 8.f;
    const float R2 = static_cast<float>(m_injectRadius * m_injectRadius);

    const bool dbg = (s_dbgFrames % DBG_LOG_INTERVAL == 1);
    int injected = 0;
    float sBefore = 0.f;
    size_t idSp = flidx(sp.x, sp.y, sp.z);
    if (dbg) sBefore = m_s[idSp];

    for (int di = -m_injectRadius; di <= m_injectRadius; ++di)
        for (int dj = 0; dj <= m_injectRadius; ++dj)
            for (int dk = -m_injectRadius; dk <= m_injectRadius; ++dk) {
                int i = sp.x + di, j = sp.y + dj, k = sp.z + dk;
                if (i < 0 || i >= m_nx || j < 0 || j >= m_ny || k < 0 || k >= m_nz) continue;
                if (isSolid(i, j, k) ) continue;
                float dx = static_cast<float>(i - sp.x), dz = static_cast<float>(k - sp.z);
                float horiz2 = dx * dx + dz * dz;
                if (m_injectCylinder) {
                    if (horiz2 > R2) continue;
                } else {
                    float dy = static_cast<float>(j - sp.y);
                    if (dx * dx + dy * dy + dz * dz > R2) continue;
                }
                size_t id = flidx(i, j, k);
                m_s[id] = std::min(1.f, m_s[id] + rate);
                m_T[id] = T_inj;
                // Główna prędkość pionowa (dym leci w górę)
                m_v[id] += vUp * rate + 0.5f * dt;
                // Mała perturbacja boczna dla naturalnego wyglądu (±0.5 jednostki/s)
                float perturbX = 0.3f * std::sin(static_cast<float>(s_dbgFrames * 0.1f + i * 0.5f));
                float perturbZ = 0.3f * std::cos(static_cast<float>(s_dbgFrames * 0.13f + k * 0.5f));
                m_u[id] += perturbX * rate;
                m_w[id] += perturbZ * rate;
                ++injected;
            }

    if (dbg) {
        std::ostringstream os;
        os << "frame=" << s_dbgFrames << " spawner=(" << m_spawnerPosition.x << "," << m_spawnerPosition.y << "," << m_spawnerPosition.z << ")"
           << " injectRaw=(" << injectCenterRaw.x << "," << injectCenterRaw.y << "," << injectCenterRaw.z << ")"
           << " injectClamp=(" << injectCenter.x << "," << injectCenter.y << "," << injectCenter.z << ")"
           << " sp=(" << sp.x << "," << sp.y << "," << sp.z << ")"
           << " injectRate=" << m_injectRate << " tempCool=" << m_tempCooling << " diff=" << m_diffusion
           << " rate=" << rate << " dt=" << dt << " injected=" << injected
           << " s@sp " << sBefore << "->" << m_s[idSp]
           << " v@sp=(" << m_u[idSp] << "," << m_v[idSp] << "," << m_w[idSp] << ")";
        dbgLog("SPAWNER", os.str());
        bool spawnerIn = m_grid.isValidWorldPosition(m_spawnerPosition);
        bool injectIn = m_grid.isValidWorldPosition(injectCenterRaw);
        dbgLog("SPAWNER", std::string("spawnerInGrid=") + (spawnerIn ? "1" : "0") + " injectRawInGrid=" + (injectIn ? "1" : "0"));
        if (injected == 0)
            dbgLog("SPAWNER", "WARN: zero cells injected (oob or solid?)");
    }
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
                    if (isGas(i + 1, j, k)) { sumS += m_s[flidx(i + 1, j, k)]; sumT += m_T[flidx(i + 1, j, k)]; ++nn; }
                    if (isGas(i - 1, j, k)) { sumS += m_s[flidx(i - 1, j, k)]; sumT += m_T[flidx(i - 1, j, k)]; ++nn; }
                    if (isGas(i, j + 1, k)) { sumS += m_s[flidx(i, j + 1, k)]; sumT += m_T[flidx(i, j + 1, k)]; ++nn; }
                    if (isGas(i, j - 1, k)) { sumS += m_s[flidx(i, j - 1, k)]; sumT += m_T[flidx(i, j - 1, k)]; ++nn; }
                    if (isGas(i, j, k + 1)) { sumS += m_s[flidx(i, j, k + 1)]; sumT += m_T[flidx(i, j, k + 1)]; ++nn; }
                    if (isGas(i, j, k - 1)) { sumS += m_s[flidx(i, j, k - 1)]; sumT += m_T[flidx(i, j, k - 1)]; ++nn; }
                    nn = std::max(1, nn);
                    float denom = 1.f + static_cast<float>(nn) * diffCoeff;
                    m_sTmp[id] = (m_s[id] + diffCoeff * sumS) / denom;
                    m_TTmp[id] = (m_T[id] + diffCoeff * sumT) / denom;
                }
        m_s.swap(m_sTmp);
        m_T.swap(m_TTmp);
    }
}

void SmokeSimulation::coolTemperature(float dt) {
    const float d = std::max(0.f, 1.f - m_tempCooling * dt);
    for (int k = 0; k < m_nz; ++k)
        for (int j = 0; j < m_ny; ++j)
            for (int i = 0; i < m_nx; ++i) {
                if (isSolid(i, j, k) ) continue;
                size_t id = flidx(i, j, k);
                m_T[id] = m_Tamb + (m_T[id] - m_Tamb) * d;
            }
}

void SmokeSimulation::addBuoyancyGravity(float dt) {
    for (int k = 0; k < m_nz; ++k)
        for (int j = 0; j < m_ny; ++j)
            for (int i = 0; i < m_nx; ++i) {
                if (isSolid(i, j, k) ) continue;
                size_t id = flidx(i, j, k);
                float b = -m_buoyancyAlpha * m_s[id] + m_buoyancyBeta * (m_T[id] - m_Tamb);
                m_v[id] += (m_gravity + b) * dt;
            }
}

void SmokeSimulation::advect(float dt) {
    const float dx = 1.f;
    const glm::vec3 o = m_grid.getMinBounds();
    m_uTmp = m_u;
    m_vTmp = m_v;
    m_wTmp = m_w;
    m_sTmp = m_s;
    m_TTmp = m_T;

    for (int k = 0; k < m_nz; ++k)
        for (int j = 0; j < m_ny; ++j)
            for (int i = 0; i < m_nx; ++i) {
                if (isSolid(i, j, k) ) continue;
                glm::vec3 pos = cellCenter(i, j, k);
                size_t id = flidx(i, j, k);
                float u0 = m_u[id], v0 = m_v[id], w0 = m_w[id];
                float s0 = m_s[id], T0 = m_T[id];
                glm::vec3 back = pos - dt * glm::vec3(u0, v0, w0);
                glm::vec3 local = (back - o) / dx;
                int ci = static_cast<int>(std::floor(local.x));
                int cj = static_cast<int>(std::floor(local.y));
                int ck = static_cast<int>(std::floor(local.z));
                ci = std::max(0, std::min(m_nx - 1, ci));
                cj = std::max(0, std::min(m_ny - 1, cj));
                ck = std::max(0, std::min(m_nz - 1, ck));
                bool atBoundary = (ci == 0 || ci == m_nx - 1 || cj == 0 || cj == m_ny - 1 || ck == 0 || ck == m_nz - 1);
                bool fromSolid = isSolid(ci, cj, ck);
                if (atBoundary || fromSolid) {
                    m_uTmp[id] = u0;
                    m_vTmp[id] = v0;
                    m_wTmp[id] = w0;
                    if (fromSolid) {
                        m_sTmp[id] = 0.f;
                        m_TTmp[id] = m_Tamb;
                    } else {
                        bool onBoundary = (i == 0 || i == m_nx - 1 || j == 0 || j == m_ny - 1 || k == 0 || k == m_nz - 1);
                        if (onBoundary) {
                            int ii = i, jj = j, kk = k;
                            if      (j == m_ny - 1 && m_ny > 1) jj = m_ny - 2;
                            else if (j == 0       && m_ny > 1) jj = 1;
                            else if (i == m_nx - 1 && m_nx > 1) ii = m_nx - 2;
                            else if (i == 0       && m_nx > 1) ii = 1;
                            else if (k == m_nz - 1 && m_nz > 1) kk = m_nz - 2;
                            else if (k == 0       && m_nz > 1) kk = 1;
                            ii = std::max(0, std::min(m_nx - 1, ii));
                            jj = std::max(0, std::min(m_ny - 1, jj));
                            kk = std::max(0, std::min(m_nz - 1, kk));
                            m_sTmp[id] = m_s[flidx(ii, jj, kk)];
                            m_TTmp[id] = m_T[flidx(ii, jj, kk)];
                        } else {
                            m_sTmp[id] = s0;
                            m_TTmp[id] = T0;
                        }
                    }
                } else {
                    m_uTmp[id] = sampleTrilinear(m_u, local.x, local.y, local.z);
                    m_vTmp[id] = sampleTrilinear(m_v, local.x, local.y, local.z);
                    m_wTmp[id] = sampleTrilinear(m_w, local.x, local.y, local.z);
                    m_sTmp[id] = sampleTrilinear(m_s, local.x, local.y, local.z);
                    m_TTmp[id] = sampleTrilinear(m_T, local.x, local.y, local.z);
                }
            }

    m_u.swap(m_uTmp);
    m_v.swap(m_vTmp);
    m_w.swap(m_wTmp);
    m_s.swap(m_sTmp);
    m_T.swap(m_TTmp);

    for (int k = 0; k < m_nz; ++k)
        for (int j = 0; j < m_ny; ++j)
            for (int i = 0; i < m_nx; ++i) {
                size_t id = flidx(i, j, k);
                if (isSolid(i, j, k)) {
                    m_u[id] = m_v[id] = m_w[id] = 0.f;
                    continue;
                }
                if (i == 0 || i == m_nx - 1) m_u[id] = 0.f;
                if (j == 0 || j == m_ny - 1) m_v[id] = 0.f;
                if (k == 0 || k == m_nz - 1) m_w[id] = 0.f;
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
                float uR = (i + 1 < m_nx) ? m_u[flidx(i + 1, j, k)] : m_u[id];
                float uL = (i - 1 >= 0) ? m_u[flidx(i - 1, j, k)] : m_u[id];
                float vT = (j + 1 < m_ny) ? m_v[flidx(i, j + 1, k)] : m_v[id];
                float vB = (j - 1 >= 0) ? m_v[flidx(i, j - 1, k)] : m_v[id];
                float wF = (k + 1 < m_nz) ? m_w[flidx(i, j, k + 1)] : m_w[id];
                float wK = (k - 1 >= 0) ? m_w[flidx(i, j, k - 1)] : m_w[id];
                if (isSolid(i + 1, j, k)) uR = m_u[id];
                if (isSolid(i - 1, j, k)) uL = m_u[id];
                if (isSolid(i, j + 1, k)) vT = m_v[id];
                if (isSolid(i, j - 1, k)) vB = m_v[id];
                if (isSolid(i, j, k + 1)) wF = m_w[id];
                if (isSolid(i, j, k - 1)) wK = m_w[id];
                div[id] = ((uR - uL) + (vT - vB) + (wF - wK)) / dx;
            }

    std::fill(m_p.begin(), m_p.end(), 0.f);
    const float dx2 = dx * dx;
    auto pNeighbor = [this](int i, int j, int k) {
        if (i < 0 || i >= m_nx || j < 0 || j >= m_ny || k < 0 || k >= m_nz) return 0.f;
        if (isSolid(i, j, k) ) return 0.f;
        return m_p[flidx(i, j, k)];
    };

    for (int it = 0; it < iterations; ++it) {
        for (int k = 0; k < m_nz; ++k)
            for (int j = 0; j < m_ny; ++j)
                for (int i = 0; i < m_nx; ++i) {
                    if (isSolid(i, j, k) ) continue;
                    float sum = pNeighbor(i + 1, j, k) + pNeighbor(i - 1, j, k)
                              + pNeighbor(i, j + 1, k) + pNeighbor(i, j - 1, k)
                              + pNeighbor(i, j, k + 1) + pNeighbor(i, j, k - 1);
                    const int nn = 6;
                    size_t id = flidx(i, j, k);
                    m_pTmp[id] = (sum - dx2 * div[id]) / static_cast<float>(nn);
                }
        m_p.swap(m_pTmp);
    }
}

void SmokeSimulation::project() {
    const float dx = 1.f;
    auto pAt = [this](int i, int j, int k) {
        if (i < 0 || i >= m_nx || j < 0 || j >= m_ny || k < 0 || k >= m_nz) return 0.f;
        if (isSolid(i, j, k) ) return 0.f;
        return m_p[flidx(i, j, k)];
    };
    for (int k = 0; k < m_nz; ++k)
        for (int j = 0; j < m_ny; ++j)
            for (int i = 0; i < m_nx; ++i) {
                if (isSolid(i, j, k) ) continue;
                size_t id = flidx(i, j, k);
                float pL = pAt(i - 1, j, k), pR = pAt(i + 1, j, k);
                float pB = pAt(i, j - 1, k), pT = pAt(i, j + 1, k);
                float pK = pAt(i, j, k - 1), pF = pAt(i, j, k + 1);
                float dpdx = (pR - pL) / (2.f * dx);
                float dpdy = (pT - pB) / (2.f * dx);
                float dpdz = (pF - pK) / (2.f * dx);
                m_u[id] -= dpdx;
                m_v[id] -= dpdy;
                m_w[id] -= dpdz;
            }

    for (int k = 0; k < m_nz; ++k)
        for (int j = 0; j < m_ny; ++j)
            for (int i = 0; i < m_nx; ++i) {
                if (isSolid(i, j, k)) {
                    size_t id = flidx(i, j, k);
                    m_u[id] = m_v[id] = m_w[id] = 0.f;
                }
                if (i == 0 || i == m_nx - 1) m_u[flidx(i, j, k)] = 0.f;
                if (j == 0 || j == m_ny - 1) m_v[flidx(i, j, k)] = 0.f;
                if (k == 0 || k == m_nz - 1) m_w[flidx(i, j, k)] = 0.f;
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
    for (float v : m_s)
        if (v > thresh) ++n;
    return n;
}
