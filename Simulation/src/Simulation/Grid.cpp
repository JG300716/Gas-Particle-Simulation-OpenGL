#include "Grid.h"
#include <algorithm>
#include <cmath>

Grid::Grid(int sizeX, int sizeY, int sizeZ)
    : m_sizeX(sizeX), m_sizeY(sizeY), m_sizeZ(sizeZ) {
    // Granice grid'a zdefiniowane przez XYZ (centrowany wokół (0,0,0))
    float hx = m_sizeX * 0.5f;
    float hy = m_sizeY * 0.5f;
    float hz = m_sizeZ * 0.5f;
    m_minBounds = glm::vec3(-hx, -hy, -hz);
    m_maxBounds = glm::vec3(hx, hy, hz);
    m_center = glm::vec3(0.0f, 0.0f, 0.0f);
}

glm::ivec3 Grid::worldToGrid(const glm::vec3& worldPos) const {
    glm::vec3 localPos = worldPos - m_minBounds;
    int x = static_cast<int>(std::floor(localPos.x));
    int y = static_cast<int>(std::floor(localPos.y));
    int z = static_cast<int>(std::floor(localPos.z));
    return glm::ivec3(x, y, z);
}

glm::vec3 Grid::gridToWorld(const glm::ivec3& gridPos) const {
    glm::vec3 localPos(
        static_cast<float>(gridPos.x) + 0.5f,
        static_cast<float>(gridPos.y) + 0.5f,
        static_cast<float>(gridPos.z) + 0.5f
    );
    return m_minBounds + localPos;
}

bool Grid::isValidPosition(const glm::ivec3& gridPos) const {
    return gridPos.x >= 0 && gridPos.x < m_sizeX &&
           gridPos.y >= 0 && gridPos.y < m_sizeY &&
           gridPos.z >= 0 && gridPos.z < m_sizeZ;
}

bool Grid::isValidWorldPosition(const glm::vec3& worldPos) const {
    return worldPos.x >= m_minBounds.x && worldPos.x <= m_maxBounds.x &&
           worldPos.y >= m_minBounds.y && worldPos.y <= m_maxBounds.y &&
           worldPos.z >= m_minBounds.z && worldPos.z <= m_maxBounds.z;
}

std::vector<glm::vec3> Grid::getGridWireframeVertices() const {
    std::vector<glm::vec3> vertices;
    
    // Dolna płaszczyzna (4 krawędzie)
    vertices.push_back(glm::vec3(m_minBounds.x, m_minBounds.y, m_minBounds.z)); // 0
    vertices.push_back(glm::vec3(m_maxBounds.x, m_minBounds.y, m_minBounds.z)); // 1
    
    vertices.push_back(glm::vec3(m_maxBounds.x, m_minBounds.y, m_minBounds.z)); // 1
    vertices.push_back(glm::vec3(m_maxBounds.x, m_minBounds.y, m_maxBounds.z)); // 2
    
    vertices.push_back(glm::vec3(m_maxBounds.x, m_minBounds.y, m_maxBounds.z)); // 2
    vertices.push_back(glm::vec3(m_minBounds.x, m_minBounds.y, m_maxBounds.z)); // 3
    
    vertices.push_back(glm::vec3(m_minBounds.x, m_minBounds.y, m_maxBounds.z)); // 3
    vertices.push_back(glm::vec3(m_minBounds.x, m_minBounds.y, m_minBounds.z)); // 0
    
    // Górna płaszczyzna (4 krawędzie)
    vertices.push_back(glm::vec3(m_minBounds.x, m_maxBounds.y, m_minBounds.z)); // 4
    vertices.push_back(glm::vec3(m_maxBounds.x, m_maxBounds.y, m_minBounds.z)); // 5
    
    vertices.push_back(glm::vec3(m_maxBounds.x, m_maxBounds.y, m_minBounds.z)); // 5
    vertices.push_back(glm::vec3(m_maxBounds.x, m_maxBounds.y, m_maxBounds.z)); // 6
    
    vertices.push_back(glm::vec3(m_maxBounds.x, m_maxBounds.y, m_maxBounds.z)); // 6
    vertices.push_back(glm::vec3(m_minBounds.x, m_maxBounds.y, m_maxBounds.z)); // 7
    
    vertices.push_back(glm::vec3(m_minBounds.x, m_maxBounds.y, m_maxBounds.z)); // 7
    vertices.push_back(glm::vec3(m_minBounds.x, m_maxBounds.y, m_minBounds.z)); // 4
    
    // Pionowe krawędzie (4 krawędzie)
    vertices.push_back(glm::vec3(m_minBounds.x, m_minBounds.y, m_minBounds.z)); // 0
    vertices.push_back(glm::vec3(m_minBounds.x, m_maxBounds.y, m_minBounds.z)); // 4
    
    vertices.push_back(glm::vec3(m_maxBounds.x, m_minBounds.y, m_minBounds.z)); // 1
    vertices.push_back(glm::vec3(m_maxBounds.x, m_maxBounds.y, m_minBounds.z)); // 5
    
    vertices.push_back(glm::vec3(m_maxBounds.x, m_minBounds.y, m_maxBounds.z)); // 2
    vertices.push_back(glm::vec3(m_maxBounds.x, m_maxBounds.y, m_maxBounds.z)); // 6
    
    vertices.push_back(glm::vec3(m_minBounds.x, m_minBounds.y, m_maxBounds.z)); // 3
    vertices.push_back(glm::vec3(m_minBounds.x, m_maxBounds.y, m_maxBounds.z)); // 7
    
    return vertices;
}
