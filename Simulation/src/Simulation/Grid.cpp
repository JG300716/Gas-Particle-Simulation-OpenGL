#include "Grid.h"
#include <algorithm>

Grid::Grid(int sizeX, int sizeY, int sizeZ, float cellSize)
    : m_sizeX(sizeX), m_sizeY(sizeY), m_sizeZ(sizeZ), m_cellSize(cellSize) {
    
    // Oblicz granice grid'a (centrowany wokół (0,0,0))
    float halfWidth = (m_sizeX * m_cellSize) / 2.0f;
    float halfHeight = (m_sizeY * m_cellSize) / 2.0f;
    float halfDepth = (m_sizeZ * m_cellSize) / 2.0f;
    
    m_minBounds = glm::vec3(-halfWidth, -halfHeight, -halfDepth);
    m_maxBounds = glm::vec3(halfWidth, halfHeight, halfDepth);
    m_center = glm::vec3(0.0f, 0.0f, 0.0f);
}

glm::ivec3 Grid::worldToGrid(const glm::vec3& worldPos) const {
    glm::vec3 localPos = worldPos - m_minBounds;
    int x = static_cast<int>(localPos.x / m_cellSize);
    int y = static_cast<int>(localPos.y / m_cellSize);
    int z = static_cast<int>(localPos.z / m_cellSize);
    
    return glm::ivec3(x, y, z);
}

glm::vec3 Grid::gridToWorld(const glm::ivec3& gridPos) const {
    glm::vec3 localPos(
        gridPos.x * m_cellSize + m_cellSize * 0.5f,
        gridPos.y * m_cellSize + m_cellSize * 0.5f,
        gridPos.z * m_cellSize + m_cellSize * 0.5f
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
