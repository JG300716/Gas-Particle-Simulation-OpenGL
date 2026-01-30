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

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool initialize(int windowWidth, int windowHeight);

    void renderSmokeVolume(const SmokeSimulation& sim, bool showTempMode = false);
    void renderObstacles(const std::vector<ObstacleDesc>& obstacles);
    void renderGridWireframe(const Grid& grid);

    void setProjection(const glm::mat4& projection);
    void setView(const glm::mat4& view);
    void clear(float r = 0.1f, float g = 0.1f, float b = 0.1f, float a = 1.0f);

    bool loadCampfire(const std::string& path);
    void renderCampfire(const glm::vec3& position, bool wireframe = false) const;

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

    glm::mat4 m_projection;
    glm::mat4 m_view;
    bool m_initialized;

    void initObstacleBuffers();
    void initGridBuffers();
    void initSmokeVolume();
};
