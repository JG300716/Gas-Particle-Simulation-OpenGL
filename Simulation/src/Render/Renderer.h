#pragma once

#include "Shader.h"
#include "GltfModel.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

class Grid;
class SmokeSimulation;

struct ObstacleDesc {
    glm::vec3 position;
    glm::vec3 size;
    glm::vec3 rotation;
    glm::vec3 scale;
};

struct LightSettings {
    float yaw{ 45.0f };      // kąt obrotu w poziomie (stopnie)
    float pitch{ 60.0f };    // kąt obrotu w pionie (stopnie)
    float distance{ 15.0f }; // odległość od środka sceny
    glm::vec3 color{ 1.0f, 1.0f, 0.8f }; // kolor światła
    bool showIndicator{ true };
    
    // Oblicz kierunek światła z kątów (od pozycji do środka)
    glm::vec3 getDirection() const;
    // Oblicz pozycję światła w świecie
    glm::vec3 getPosition() const;
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool initialize(int windowWidth, int windowHeight);

    void renderSmokeVolume(const SmokeSimulation& sim, bool showTempMode = false);
    void renderObstacles(const std::vector<ObstacleDesc>& obstacles);
    void renderGridWireframe(const Grid& grid);
    void renderLightIndicator();

    void setProjection(const glm::mat4& projection);
    void setView(const glm::mat4& view);
    void clear(float r = 0.1f, float g = 0.1f, float b = 0.1f, float a = 1.0f);

    bool loadCampfire(const std::string& path);
    void renderCampfire(const glm::vec3& position, bool wireframe = false) const;

    LightSettings& getLightSettings() { return m_light; }
    const LightSettings& getLightSettings() const { return m_light; }

    static std::string findCampfirePath();

private:
    Shader m_obstacleShader;
    Shader m_gridShader;
    Shader m_modelShader;
    Shader m_smokeShader;
    GltfModel m_campfire;

    GLuint m_obstacleVAO;
    GLuint m_obstacleVBO;
    GLuint m_gridVAO;
    GLuint m_gridVBO;
    GLuint m_smokeVAO;
    GLuint m_smoke3D;
    GLuint m_smoke3DTemp;
    GLuint m_sphereVAO;
    GLuint m_sphereVBO;
    GLuint m_sphereEBO;
    int m_sphereIndexCount;

    glm::mat4 m_projection;
    glm::mat4 m_view;
    bool m_initialized;
    LightSettings m_light;

    void initObstacleBuffers();
    void initGridBuffers();
    void initSmokeVolume();
    void initSphereBuffers();
};
