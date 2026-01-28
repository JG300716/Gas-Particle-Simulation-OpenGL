#include "SmokeSimulation.h"
#include <algorithm>
#include <random>
#include <iterator>
#include <cstddef>

SmokeSimulation::SmokeSimulation() 
    : m_grid(40, 40, 40, 1.0f),
      m_ambientPressure(1013.25f),  // Domyślne ciśnienie atmosferyczne [hPa]
      m_particleDensity(0.001225f),  // Domyślna gęstość powietrza [g/cm^3] = 1.225 kg/m^3
      m_gravity(-9.81f),              // Domyślna grawitacja [m/s^2]
      m_dampingFactor(0.95f),
      m_timeScale(1.0f),              // Domyślny modyfikator czasu (normalna prędkość)
      m_simulationWidth(40),
      m_simulationHeight(40),
      m_simulationDepth(40),
      m_spawnerPosition(0.0f, 0.0f, 0.0f) {
}

SmokeSimulation::~SmokeSimulation() {
}

bool SmokeSimulation::initialize(const UINT32& simulationWidth, const UINT32& simulationHeight, const UINT32& simulationDepth, const float& cellSize) {
    m_simulationWidth = static_cast<int>(simulationWidth);
    m_simulationHeight = static_cast<int>(simulationHeight);
    m_simulationDepth = static_cast<int>(simulationDepth);

    int gridX = static_cast<int>(simulationWidth);
    int gridY = static_cast<int>(simulationHeight);
    int gridZ = static_cast<int>(simulationDepth);
    m_grid = Grid(gridX, gridY, gridZ, cellSize);
    updateGridStrides();

    m_gridOccupancyFlat.assign(static_cast<size_t>(m_totalCells), OCCUPANCY_EMPTY);
    generateInitialParticles();
    updateGridOccupancy();

    return true;
}

void SmokeSimulation::setGridSize(int sizeX, int sizeY, int sizeZ) {
    m_gasParticles.erase(
        std::remove_if(m_gasParticles.begin(), m_gasParticles.end(),
            [this, sizeX, sizeY, sizeZ](const GasParticle& p) {
                glm::ivec3 gp = m_grid.worldToGrid(p.position);
                return gp.x >= sizeX || gp.y >= sizeY || gp.z >= sizeZ || gp.x < 0 || gp.y < 0 || gp.z < 0;
            }),
        m_gasParticles.end()
    );

    m_grid = Grid(sizeX, sizeY, sizeZ, m_grid.getCellSize());
    m_simulationWidth = sizeX;
    m_simulationHeight = sizeY;
    m_simulationDepth = sizeZ;
    updateGridStrides();
    updateGridOccupancy();
}

void SmokeSimulation::generateInitialParticles() {
    m_gasParticles.clear();
    const float cellSize = m_grid.getCellSize();
    const int maxParticles = std::min(80000, m_totalCells);
    m_gasParticles.reserve(static_cast<size_t>(maxParticles));

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> velDist(-0.5f, 0.5f);
    std::uniform_real_distribution<float> chanceDist(0.0f, 1.0f);
    const float fillChance = std::min(1.0f, maxParticles * 1.5f / static_cast<float>(m_totalCells));

    int added = 0;
    const int sx = m_grid.getSizeX(), sy = m_grid.getSizeY(), sz = m_grid.getSizeZ();
    for (int x = 0; x < sx && added < maxParticles; ++x) {
        for (int y = 0; y < sy && added < maxParticles; ++y) {
            for (int z = 0; z < sz && added < maxParticles; ++z) {
                if (chanceDist(gen) <= (1.0f - fillChance)) continue;
                glm::ivec3 gridPos(x, y, z);
                if (!canPlaceParticleAt(gridPos)) continue;
                GasParticle p;
                p.position = m_grid.gridToWorld(gridPos);
                p.velocity = glm::vec3(velDist(gen), velDist(gen), velDist(gen));
                p.density = m_particleDensity;
                p.pressure = m_ambientPressure;
                p.temperature = 293.15f;
                p.size = cellSize;
                m_gasParticles.push_back(p);
                ++added;
            }
        }
    }
}

void SmokeSimulation::run(float deltaTime) {
    float scaledDeltaTime = deltaTime * m_timeScale;
    
    // Jeśli czas jest zatrzymany, nie aktualizuj symulacji
    if (m_timeScale <= 0.0f) {
        return;
    }
    
    updateParticles(scaledDeltaTime);
    //applyParticleInteractions();
    checkCollisions();
    updateGridOccupancy();
}

void SmokeSimulation::updateParticles(float deltaTime) {
    constexpr float maxVelocity = 10.0f;
    const float cellSize = m_grid.getCellSize();
    const glm::vec3 boundsMin = m_grid.getMinBounds();
    const glm::vec3 boundsMax = m_grid.getMaxBounds();
    const float halfH = m_simulationHeight * 0.5f;

    const size_t n = m_gasParticles.size();
    for (size_t i = 0; i < n; ++i) {
        GasParticle& particle = m_gasParticles[i];

        float buoyancyForce = (m_ambientPressure - particle.pressure) * 0.001f;
        particle.velocity.y += (m_gravity + buoyancyForce) * deltaTime;

        float speed = glm::length(particle.velocity);
        if (speed > maxVelocity)
            particle.velocity = glm::normalize(particle.velocity) * maxVelocity;

        const glm::vec3 newPosition = particle.position + particle.velocity * deltaTime;

        if (m_grid.isValidWorldPosition(newPosition)) {
            const glm::ivec3 newGridPos = m_grid.worldToGrid(newPosition);
            const int newKey = gridPosToKey(newGridPos);
            const size_t occ = (newKey >= 0 && newKey < m_totalCells) ? m_gridOccupancyFlat[newKey] : OCCUPANCY_EMPTY;

            const glm::ivec3 oldGridPos = m_grid.worldToGrid(particle.position);
            const int oldKey = gridPosToKey(oldGridPos);

            if (occ == OCCUPANCY_EMPTY || occ == i || newKey == oldKey) {
                const glm::vec3 cellCenter = m_grid.gridToWorld(newGridPos);
                const float distToCenter = glm::length(newPosition - cellCenter);
                if (newKey == oldKey || distToCenter < cellSize * 0.4f)
                    particle.position = newPosition;
                else
                    particle.position = cellCenter;
            } else {
                const glm::ivec3 freeNeighbor = findFreeNeighborCell(oldGridPos, particle.velocity);
                if (freeNeighbor != oldGridPos && m_grid.isValidPosition(freeNeighbor)) {
                    particle.position = m_grid.gridToWorld(freeNeighbor);
                    particle.velocity.x *= 0.7f;
                    particle.velocity.z *= 0.7f;
                } else {
                    particle.velocity.x = 0.0f;
                    particle.velocity.z = 0.0f;
                    particle.position = m_grid.gridToWorld(oldGridPos);
                }
            }
        } else {
            if (newPosition.x < boundsMin.x || newPosition.x > boundsMax.x) {
                particle.velocity.x *= -0.5f;
                particle.position.x = std::clamp(particle.position.x, boundsMin.x, boundsMax.x);
            }
            if (newPosition.y < boundsMin.y || newPosition.y > boundsMax.y) {
                particle.velocity.y = 0.0f;
                particle.position.y = std::clamp(particle.position.y, boundsMin.y, boundsMax.y);
            }
            if (newPosition.z < boundsMin.z || newPosition.z > boundsMax.z) {
                particle.velocity.z *= -0.5f;
                particle.position.z = std::clamp(particle.position.z, boundsMin.z, boundsMax.z);
            }
        }

        particle.velocity *= m_dampingFactor;
        const float heightFactor = (particle.position.y + halfH) / static_cast<float>(m_simulationHeight);
        particle.pressure = m_ambientPressure * (1.0f - heightFactor * 0.1f);
    }
}

void SmokeSimulation::checkCollisions() {
    constexpr float boundaryMargin = 0.1f;
    const float hw = m_simulationWidth * 0.5f;
    const float hh = m_simulationHeight * 0.5f;
    const float hd = m_simulationDepth * 0.5f;
    const float xMin = -hw + boundaryMargin, xMax = hw - boundaryMargin;
    const float yMin = -hh + boundaryMargin, yMax = hh - boundaryMargin;
    const float zMin = -hd + boundaryMargin, zMax = hd - boundaryMargin;

    for (GasParticle& p : m_gasParticles) {
        if (p.position.x < xMin) { p.position.x = xMin; p.velocity.x *= -0.5f; }
        if (p.position.x > xMax) { p.position.x = xMax; p.velocity.x *= -0.5f; }
        if (p.position.y < yMin) { p.position.y = yMin; p.velocity.y = 0.0f; }
        if (p.position.y > yMax) { p.position.y = yMax; p.velocity.y = 0.0f; }
        if (p.position.z < zMin) { p.position.z = zMin; p.velocity.z *= -0.5f; }
        if (p.position.z > zMax) { p.position.z = zMax; p.velocity.z *= -0.5f; }

        for (const Obstacle& obs : m_obstacles) {
            const glm::vec3 half = obs.size * 0.5f;
            const float minX = obs.position.x - half.x, maxX = obs.position.x + half.x;
            const float minY = obs.position.y - half.y, maxY = obs.position.y + half.y;
            const float minZ = obs.position.z - half.z, maxZ = obs.position.z + half.z;
            if (p.position.x < minX || p.position.x > maxX || p.position.y < minY || p.position.y > maxY ||
                p.position.z < minZ || p.position.z > maxZ)
                continue;

            glm::vec3 toP = p.position - obs.position;
            const float dX = std::min(std::abs(p.position.x - minX), std::abs(p.position.x - maxX));
            const float dY = std::min(std::abs(p.position.y - minY), std::abs(p.position.y - maxY));
            const float dZ = std::min(std::abs(p.position.z - minZ), std::abs(p.position.z - maxZ));
            glm::vec3 n(0.0f);
            if (dX < dY && dX < dZ)      n = glm::vec3(toP.x > 0 ? 1.0f : -1.0f, 0.0f, 0.0f);
            else if (dY < dZ)            n = glm::vec3(0.0f, toP.y > 0 ? 1.0f : -1.0f, 0.0f);
            else                         n = glm::vec3(0.0f, 0.0f, toP.z > 0 ? 1.0f : -1.0f);

            p.velocity = p.velocity - 2.0f * glm::dot(p.velocity, n) * n;
            p.velocity *= 0.7f;
            p.position += n * 0.1f;
            break;
        }
    }
}

bool SmokeSimulation::isPointInObstacle(const glm::vec3& point) const {
    for (const auto& obstacle : m_obstacles) {
        glm::vec3 halfSize = obstacle.size * 0.5f;
        glm::vec3 min = obstacle.position - halfSize;
        glm::vec3 max = obstacle.position + halfSize;
        
        if (point.x >= min.x && point.x <= max.x &&
            point.y >= min.y && point.y <= max.y &&
            point.z >= min.z && point.z <= max.z) {
            return true;
        }
    }
    return false;
}

glm::vec3 SmokeSimulation::getCollisionNormal(const glm::vec3& point, const Obstacle& obstacle) const {
    glm::vec3 toPoint = point - obstacle.position;
    
    // Zwróć normalną w kierunku najbliższej ściany
    glm::vec3 halfSize = obstacle.size * 0.5f;
    float distX = std::min(std::abs(point.x - (obstacle.position.x - halfSize.x)), 
                          std::abs(point.x - (obstacle.position.x + halfSize.x)));
    float distY = std::min(std::abs(point.y - (obstacle.position.y - halfSize.y)), 
                          std::abs(point.y - (obstacle.position.y + halfSize.y)));
    float distZ = std::min(std::abs(point.z - (obstacle.position.z - halfSize.z)), 
                          std::abs(point.z - (obstacle.position.z + halfSize.z)));
    
    if (distX < distY && distX < distZ) {
        return glm::vec3(toPoint.x > 0 ? 1.0f : -1.0f, 0.0f, 0.0f);
    } else if (distY < distZ) {
        return glm::vec3(0.0f, toPoint.y > 0 ? 1.0f : -1.0f, 0.0f);
    } else {
        return glm::vec3(0.0f, 0.0f, toPoint.z > 0 ? 1.0f : -1.0f);
    }
}

void SmokeSimulation::addObstacle(const Obstacle& obstacle) {
    m_obstacles.push_back(obstacle);
}

void SmokeSimulation::removeObstacle(size_t index) {
    if (index < m_obstacles.size()) {
        m_obstacles.erase(m_obstacles.begin() + index);
    }
}

void SmokeSimulation::clearObstacles() {
    m_obstacles.clear();
}

void SmokeSimulation::addParticlesAt(const glm::vec3& position, int count) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> velDist(-1.0f, 1.0f);
    
    glm::ivec3 centerGridPos = m_grid.worldToGrid(position);


    const float cellSize = m_grid.getCellSize();
    int added = 0;
    for (int offset = 0; offset < 10 && added < count; ++offset) {
        for (int dx = -offset; dx <= offset && added < count; ++dx) {
            for (int dy = -offset; dy <= offset && added < count; ++dy) {
                for (int dz = -offset; dz <= offset && added < count; ++dz) {
                    glm::ivec3 gridPos = centerGridPos + glm::ivec3(dx, dy, dz);
                    
                    if (canPlaceParticleAt(gridPos)) {
                        GasParticle particle;
                        particle.position = m_grid.gridToWorld(gridPos);
                        particle.velocity = glm::vec3(velDist(gen), velDist(gen), velDist(gen));
                        particle.density = m_particleDensity;
                        particle.pressure = m_ambientPressure;
                        particle.temperature = 293.15f;
                        particle.size = cellSize * 5.f;
                        m_gasParticles.push_back(particle);
                        added++;
                    }
                }
            }
        }
    }
    
    updateGridOccupancy();
}

void SmokeSimulation::spawnParticles(int count) {
    addParticlesAt(m_spawnerPosition, count);
}

int SmokeSimulation::gridPosToKey(const glm::ivec3& gridPos) const {
    return gridPos.x + gridPos.y * m_strideY + gridPos.z * m_strideZ;
}

glm::ivec3 SmokeSimulation::keyToGridPos(int key) const {
    const int z = key / m_strideZ;
    const int remainder = key % m_strideZ;
    const int y = remainder / m_strideY;
    const int x = remainder % m_strideY;
    return { x, y, z };
}

bool SmokeSimulation::canPlaceParticleAt(const glm::ivec3& gridPos) const {
    if (!m_grid.isValidPosition(gridPos)) return false;
    const int key = gridPosToKey(gridPos);
    return key >= 0 && key < m_totalCells && m_gridOccupancyFlat[key] == OCCUPANCY_EMPTY;
}

glm::ivec3 SmokeSimulation::findFreeNeighborCell(const glm::ivec3& gridPos, const glm::vec3& velocity) const {
    glm::ivec3 const direction = glm::ivec3(
        velocity.x > 0.1f ? 1 : (velocity.x < -0.1f ? -1 : 0),
        velocity.y > 0.1f ? 1 : (velocity.y < -0.1f ? -1 : 0),
        velocity.z > 0.1f ? 1 : (velocity.z < -0.1f ? -1 : 0)
    );
    
    glm::ivec3 candidates[] = {
        gridPos + direction, // Główny kierunek
        gridPos + glm::ivec3(direction.x, 0, 0), // Tylko X
        gridPos + glm::ivec3(0, direction.y, 0), // Tylko Y
        gridPos + glm::ivec3(0, 0, direction.z), // Tylko Z
        gridPos + glm::ivec3(direction.x, direction.y, 0), // X+Y
        gridPos + glm::ivec3(direction.x, 0, direction.z), // X+Z
        gridPos + glm::ivec3(0, direction.y, direction.z), // Y+Z
    };
    
    for (const auto& candidate : candidates) {
        if (canPlaceParticleAt(candidate)) {
            return candidate;
        }
    }
    const float cellSize = m_grid.getCellSize();
    const float cellJump = cellSize / 10;
    const int dy = direction.y;
    for (float dx = -cellSize; dx <= cellSize; dx+=cellJump) {
        for (float dz = -cellSize; dz <= cellSize; dz+=cellJump) {
            if (dx == 0.f && dz == 0.f) continue;
            
            const glm::ivec3 neighbor = gridPos + glm::ivec3(dx, dy, dz);
            if (canPlaceParticleAt(neighbor)) {
                return neighbor;
            }
        }
    }
    
    return gridPos;
}

void SmokeSimulation::updateGridOccupancy() {
    m_gridOccupancyFlat.assign(static_cast<size_t>(m_totalCells), OCCUPANCY_EMPTY);
    std::vector<size_t> toRemove;
    toRemove.reserve(256);

    for (size_t i = 0; i < m_gasParticles.size(); ++i) {
        const glm::ivec3 gp = m_grid.worldToGrid(m_gasParticles[i].position);
        if (!m_grid.isValidPosition(gp)) continue;
        const int key = gridPosToKey(gp);
        if (key < 0 || key >= m_totalCells) continue;
        if (m_gridOccupancyFlat[key] != OCCUPANCY_EMPTY) {
            toRemove.push_back(i);
            continue;
        }
        m_gridOccupancyFlat[key] = i;
    }

    if (toRemove.empty()) return;

    std::ranges::sort(toRemove.begin(), toRemove.end(), [](const size_t &a, const size_t &b) { return a > b; });
    for (const size_t &idx : toRemove) {
        std::swap(m_gasParticles[idx], m_gasParticles.back());
        m_gasParticles.pop_back();
    }

    m_gridOccupancyFlat.assign(static_cast<size_t>(m_totalCells), OCCUPANCY_EMPTY);
    for (size_t i = 0; i < m_gasParticles.size(); ++i) {
        const glm::ivec3 gp = m_grid.worldToGrid(m_gasParticles[i].position);
        if (!m_grid.isValidPosition(gp)) continue;
        const int key = gridPosToKey(gp);
        if (key >= 0 && key < m_totalCells)
            m_gridOccupancyFlat[key] = i;
    }
}

void SmokeSimulation::updateGridStrides() {
    const int sx = m_grid.getSizeX(), sy = m_grid.getSizeY(), sz = m_grid.getSizeZ();
    m_strideY = sx;
    m_strideZ = sx * sy;
    m_totalCells = sx * sy * sz;
}

void SmokeSimulation::applyParticleInteractions() {
    // Sprawdź czy cząsteczki są w sąsiednich komórkach grid'a
    for (size_t i = 0; i < m_gasParticles.size(); ++i) {
        auto& particle1 = m_gasParticles[i];
        glm::ivec3 gridPos1 = m_grid.worldToGrid(particle1.position);
        
        for (size_t j = i + 1; j < m_gasParticles.size(); ++j) {
            auto& particle2 = m_gasParticles[j];
            glm::ivec3 gridPos2 = m_grid.worldToGrid(particle2.position);
            
            // Sprawdź czy są w tej samej lub sąsiednich komórkach
            glm::ivec3 diff = gridPos1 - gridPos2;
            int manhattanDist = std::abs(diff.x) + std::abs(diff.y) + std::abs(diff.z);
            
            // Jeśli są w tej samej komórce lub bezpośrednio sąsiednich (Manhattan distance <= 1)
            // to nie dodawaj sił odpychających - pozwól grid'owi to obsłużyć
            if (manhattanDist <= 1) {
                // Wyzeruj prędkość w kierunku drugiej cząsteczki, żeby nie skakały
                glm::vec3 posDiff = particle1.position - particle2.position;
                float distance = glm::length(posDiff);
                
                if (distance < m_grid.getCellSize() && distance > 0.001f) {
                    // Tylko w osiach X i Z - w osi Y wszystkie cząsteczki mają podobną prędkość
                    glm::vec3 direction = posDiff / distance;
                    // Wyzeruj składową Y kierunku (tylko XZ)
                    direction.y = 0.0f;
                    float dirLength = glm::length(direction);
                    
                    if (dirLength > 0.001f) {
                        direction = glm::normalize(direction);
                        
                        // Wyzeruj tylko składowe X i Z prędkości
                        glm::vec3 velXZ1 = glm::vec3(particle1.velocity.x, 0.0f, particle1.velocity.z);
                        float velDot1 = glm::dot(velXZ1, direction);
                        if (velDot1 > 0.0f) {
                            // Tylko jeśli porusza się w kierunku drugiej cząsteczki (w płaszczyźnie XZ)
                            particle1.velocity.x -= direction.x * velDot1 * 0.5f;
                            particle1.velocity.z -= direction.z * velDot1 * 0.5f;
                        }
                        
                        // To samo dla drugiej cząsteczki
                        glm::vec3 velXZ2 = glm::vec3(particle2.velocity.x, 0.0f, particle2.velocity.z);
                        float velDot2 = glm::dot(velXZ2, -direction);
                        if (velDot2 > 0.0f) {
                            particle2.velocity.x -= (-direction.x) * velDot2 * 0.5f;
                            particle2.velocity.z -= (-direction.z) * velDot2 * 0.5f;
                        }
                    }
                }
            }
        }
    }
}
