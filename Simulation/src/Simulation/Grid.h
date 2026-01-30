#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <optional>

// Struktura reprezentująca grid 3D
class Grid {
public:
    Grid(int sizeX, int sizeY, int sizeZ);
    
    // Pobierz rozmiary grid'a
    int getSizeX() const { return m_sizeX; }
    int getSizeY() const { return m_sizeY; }
    int getSizeZ() const { return m_sizeZ; }
    
    // Konwersja pozycji świata na indeksy grid'a
    glm::ivec3 worldToGrid(const glm::vec3& worldPos) const;
    
    // Konwersja indeksów grid'a na pozycję świata (środek komórki)
    glm::vec3 gridToWorld(const glm::ivec3& gridPos) const;
    
    // Sprawdź czy pozycja jest w granicach grid'a
    bool isValidPosition(const glm::ivec3& gridPos) const;
    bool isValidWorldPosition(const glm::vec3& worldPos) const;
    
    // Pobierz granice grid'a w przestrzeni świata
    glm::vec3 getMinBounds() const { return m_minBounds; }
    glm::vec3 getMaxBounds() const { return m_maxBounds; }
    
    // Pobierz wierzchołki do renderowania konturów grid'a
    std::vector<glm::vec3> getGridWireframeVertices() const;

private:
    int m_sizeX, m_sizeY, m_sizeZ;
    glm::vec3 m_minBounds;
    glm::vec3 m_maxBounds;
    glm::vec3 m_center;
};
