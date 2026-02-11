#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <optional>

class Grid {
public:
    Grid(int sizeX, int sizeY, int sizeZ);
    
    [[nodiscard]] int getSizeX() const { return m_sizeX; }
    [[nodiscard]] int getSizeY() const { return m_sizeY; }
    [[nodiscard]] int getSizeZ() const { return m_sizeZ; }
    
    [[nodiscard]] glm::ivec3 worldToGrid(const glm::vec3& worldPos) const;
    [[nodiscard]] glm::vec3 gridToWorld(const glm::ivec3& gridPos) const;
    
    [[nodiscard]] bool isValidPosition(const glm::ivec3& gridPos) const;
    [[nodiscard]] bool isValidWorldPosition(const glm::vec3& worldPos) const;
    
    [[nodiscard]] glm::vec3 getMinBounds() const { return m_minBounds; }
    [[nodiscard]] glm::vec3 getMaxBounds() const { return m_maxBounds; }
    
    [[nodiscard]] std::vector<glm::vec3> getGridWireframeVertices() const;

private:
    int m_sizeX, m_sizeY, m_sizeZ;
    glm::vec3 m_minBounds;
    glm::vec3 m_maxBounds;
    glm::vec3 m_center;
};
