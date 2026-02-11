#include "Renderer.h"
#include "../Simulation/Grid.h"
#include "../Simulation/SmokeSimulation.h"
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include <iostream>
#include <filesystem>
#include <cmath>

glm::vec3 LightSettings::getDirection() const {
    return glm::normalize(getPosition());
}

glm::vec3 LightSettings::getPosition() const {
    // Sferyczne współrzędne -> kartezjańskie
    float yawRad = glm::radians(yaw);
    float pitchRad = glm::radians(pitch);
    float x = distance * cos(pitchRad) * sin(yawRad);
    float y = distance * sin(pitchRad);
    float z = distance * cos(pitchRad) * cos(yawRad);
    return glm::vec3(x, y, z);
}

Renderer::Renderer() : m_initialized(false), m_obstacleVAO(0), m_obstacleVBO(0),
                       m_gridVAO(0), m_gridVBO(0), m_smokeVAO(0), m_smoke3D(0), m_smoke3DTemp(0),
                       m_sphereVAO(0), m_sphereVBO(0), m_sphereEBO(0), m_sphereIndexCount(0) {
    m_projection = glm::mat4(1.0f);
    m_view = glm::mat4(1.0f);
}

Renderer::~Renderer() {
    if (m_obstacleVAO != 0) glDeleteVertexArrays(1, &m_obstacleVAO);
    if (m_obstacleVBO != 0) glDeleteBuffers(1, &m_obstacleVBO);
    if (m_gridVAO != 0) glDeleteVertexArrays(1, &m_gridVAO);
    if (m_gridVBO != 0) glDeleteBuffers(1, &m_gridVBO);
    if (m_smokeVAO != 0) glDeleteVertexArrays(1, &m_smokeVAO);
    if (m_smoke3D != 0) glDeleteTextures(1, &m_smoke3D);
    if (m_smoke3DTemp != 0) glDeleteTextures(1, &m_smoke3DTemp);
    if (m_sphereVAO != 0) glDeleteVertexArrays(1, &m_sphereVAO);
    if (m_sphereVBO != 0) glDeleteBuffers(1, &m_sphereVBO);
    if (m_sphereEBO != 0) glDeleteBuffers(1, &m_sphereEBO);
}

bool Renderer::initialize(int windowWidth, int windowHeight) {
    // Znajdź ścieżkę do shaderów (względem katalogu wykonywalnego)
    std::filesystem::path exePath = std::filesystem::current_path();
    std::filesystem::path shaderDir = exePath / "shaders";
    
    // Jeśli shadery nie są w katalogu wykonywalnym, spróbuj względem źródła
    if (!std::filesystem::exists(shaderDir)) {
        // Spróbuj kilka możliwych ścieżek
        std::filesystem::path possiblePaths[] = {
            exePath.parent_path().parent_path() / "Simulation" / "shaders",
            exePath / ".." / ".." / "Simulation" / "shaders",
            exePath / ".." / "Simulation" / "shaders",
            exePath.parent_path() / "shaders"
        };
        
        bool found = false;
        for (const auto& path : possiblePaths) {
            try {
                if (std::filesystem::exists(path)) {
                    shaderDir = std::filesystem::canonical(path);
                    found = true;
                    break;
                }
            } catch (...) {
                // Ignore errors when checking paths
            }
        }
        
        if (!found) {
            std::cerr << "Warning: Could not find shaders directory. Trying: " << shaderDir << std::endl;
        }
    }
    
    std::string shaderPath = shaderDir.string();

    std::string obstacleVert = shaderPath + "/obstacle.vert";
    std::string obstacleFrag = shaderPath + "/obstacle.frag";
    if (!m_obstacleShader.loadFromFile(obstacleVert, obstacleFrag)) {
        std::cerr << "Failed to load obstacle shader from: " << obstacleVert << ", " << obstacleFrag << std::endl;
        return false;
    }

    std::string gridVert = shaderPath + "/grid.vert";
    std::string gridFrag = shaderPath + "/grid.frag";
    if (!m_gridShader.loadFromFile(gridVert, gridFrag)) {
        std::cerr << "Failed to load grid shader from: " << gridVert << ", " << gridFrag << std::endl;
        return false;
    }

    std::string modelVert = shaderPath + "/model.vert";
    std::string modelFrag = shaderPath + "/model.frag";
    if (!m_modelShader.loadFromFile(modelVert, modelFrag)) {
        std::cerr << "Failed to load model shader from: " << modelVert << ", " << modelFrag << std::endl;
        return false;
    }

    std::string smokeVert = shaderPath + "/smoke_raymarch.vert";
    std::string smokeFrag = shaderPath + "/smoke_raymarch.frag";
    if (!m_smokeShader.loadFromFile(smokeVert, smokeFrag)) {
        std::cerr << "Failed to load smoke shader from: " << smokeVert << ", " << smokeFrag << std::endl;
        return false;
    }

    initObstacleBuffers();
    initGridBuffers();
    initSmokeVolume();
    initSphereBuffers();

    std::string campfirePath = findCampfirePath();
    if (!campfirePath.empty() && loadCampfire(campfirePath))
        std::cout << "Campfire loaded: " << campfirePath << std::endl;
    else if (campfirePath.empty())
        std::cerr << "Campfire not found (add Campfire/campfire.glb next to exe)" << std::endl;

    // Macierz projekcji będzie ustawiana przez kamerę
    m_projection = glm::mat4(1.0f);

    m_initialized = true;
    return true;
}

void Renderer::initObstacleBuffers() {
    glGenVertexArrays(1, &m_obstacleVAO);
    glGenBuffers(1, &m_obstacleVBO);

    glBindVertexArray(m_obstacleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_obstacleVBO);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);

    glBindVertexArray(0);
}

void Renderer::initSmokeVolume() {
    glGenVertexArrays(1, &m_smokeVAO);
    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    float quad[] = { -1.f, -1.f, 1.f, -1.f, -1.f, 1.f, -1.f, 1.f, 1.f, -1.f, 1.f, 1.f };
    glBindVertexArray(m_smokeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
}

void Renderer::renderSmokeVolume(const SmokeSimulation& sim, bool showTempMode) {
    if (!m_initialized) return;
    int nx = sim.getSmokeNx(), ny = sim.getSmokeNy(), nz = sim.getSmokeNz();
    if (nx <= 0 || ny <= 0 || nz <= 0) return;
    const float* density = sim.getSmokeDensityData();
    if (!density) return;

    glm::vec3 vmin = sim.getGrid().getMinBounds();
    glm::vec3 vmax = sim.getGrid().getMaxBounds();

    if (m_smoke3D == 0) glGenTextures(1, &m_smoke3D);
    glBindTexture(GL_TEXTURE_3D, m_smoke3D);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, nx, ny, nz, 0, GL_RED, GL_FLOAT, density);
    glBindTexture(GL_TEXTURE_3D, 0);

    if (showTempMode) {
        const float* temp = sim.getSmokeTemperatureData();
        if (temp) {
            if (m_smoke3DTemp == 0) glGenTextures(1, &m_smoke3DTemp);
            glBindTexture(GL_TEXTURE_3D, m_smoke3DTemp);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, nx, ny, nz, 0, GL_RED, GL_FLOAT, temp);
            glBindTexture(GL_TEXTURE_3D, 0);
        }
    }

    glm::mat4 viewProj = m_projection * m_view;
    glm::mat4 invViewProj = glm::inverse(viewProj);
    glm::mat4 invView = glm::inverse(m_view);
    glm::vec3 camPos(invView[3][0], invView[3][1], invView[3][2]);

    m_smokeShader.use();
    m_smokeShader.setMat4("uInvViewProj", invViewProj);
    m_smokeShader.setVec3("uCamPos", camPos);
    m_smokeShader.setVec3("uVolumeMin", vmin);
    m_smokeShader.setVec3("uVolumeMax", vmax);
    m_smokeShader.setInt("uDensity", 0);
    m_smokeShader.setBool("uShowTemp", showTempMode);
    if (showTempMode) {
        m_smokeShader.setInt("uTemperature", 1);
        float tMin = sim.getTempAmbient();
        float tMax = tMin + 80.f;
        m_smokeShader.setFloat("uTempMin", tMin);
        m_smokeShader.setFloat("uTempMax", tMax);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, m_smoke3D);
    if (showTempMode && m_smoke3DTemp != 0) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_3D, m_smoke3DTemp);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(m_smokeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_3D, 0);
    if (showTempMode && m_smoke3DTemp != 0) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_3D, 0);
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, 0);
}

void Renderer::initSphereBuffers() {
    const int stacks = 16;
    const int slices = 16;
    const float radius = 0.5f;
    
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    for (int i = 0; i <= stacks; ++i) {
        float phi = glm::pi<float>() * float(i) / float(stacks);
        for (int j = 0; j <= slices; ++j) {
            float theta = 2.0f * glm::pi<float>() * float(j) / float(slices);
            float x = radius * sin(phi) * cos(theta);
            float y = radius * cos(phi);
            float z = radius * sin(phi) * sin(theta);
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
    }
    
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            int first = i * (slices + 1) + j;
            int second = first + slices + 1;
            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);
            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }
    
    m_sphereIndexCount = static_cast<int>(indices.size());
    
    glGenVertexArrays(1, &m_sphereVAO);
    glGenBuffers(1, &m_sphereVBO);
    glGenBuffers(1, &m_sphereEBO);
    
    glBindVertexArray(m_sphereVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, m_sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    
    glBindVertexArray(0);
}

void Renderer::renderLightIndicator() {
    if (!m_initialized || !m_light.showIndicator) return;
    
    glm::vec3 lightPos = m_light.getPosition();
    
    glm::mat4 model = glm::translate(glm::mat4(1.0f), lightPos);
    model = glm::scale(model, glm::vec3(1.0f));
    
    m_obstacleShader.use();
    m_obstacleShader.setMat4("uProjection", m_projection);
    m_obstacleShader.setMat4("uView", m_view * model);
    m_obstacleShader.setVec3("uColor", glm::vec3(1.0f, 1.0f, 0.0f));
    
    glBindVertexArray(m_sphereVAO);
    glDrawElements(GL_TRIANGLES, m_sphereIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Renderer::renderObstacles(const std::vector<ObstacleDesc>& obstacles) {
    if (!m_initialized || obstacles.empty()) return;

    m_obstacleShader.use();
    m_obstacleShader.setMat4("uProjection", m_projection);
    m_obstacleShader.setMat4("uView", m_view);
    m_obstacleShader.setVec3("uColor", glm::vec3(0.5f, 0.5f, 0.5f));

    std::vector<float> vertices;
    vertices.reserve(36 * obstacles.size());

    for (const auto& o : obstacles) {
        glm::vec3 h = o.size * 0.5f * o.scale;
        glm::mat4 R = glm::eulerAngleXYZ(
            glm::radians(o.rotation.x), glm::radians(o.rotation.y), glm::radians(o.rotation.z));
        glm::vec3 c[8] = {
            o.position + glm::vec3(R * glm::vec4(-h.x, -h.y, -h.z, 0.f)),
            o.position + glm::vec3(R * glm::vec4( h.x, -h.y, -h.z, 0.f)),
            o.position + glm::vec3(R * glm::vec4( h.x,  h.y, -h.z, 0.f)),
            o.position + glm::vec3(R * glm::vec4(-h.x,  h.y, -h.z, 0.f)),
            o.position + glm::vec3(R * glm::vec4(-h.x, -h.y,  h.z, 0.f)),
            o.position + glm::vec3(R * glm::vec4( h.x, -h.y,  h.z, 0.f)),
            o.position + glm::vec3(R * glm::vec4( h.x,  h.y,  h.z, 0.f)),
            o.position + glm::vec3(R * glm::vec4(-h.x,  h.y,  h.z, 0.f))
        };
        auto push = [&vertices](const glm::vec3& v) {
            vertices.push_back(v.x); vertices.push_back(v.y); vertices.push_back(v.z);
        };
        push(c[4]); push(c[5]); push(c[6]); push(c[4]); push(c[6]); push(c[7]);
        push(c[1]); push(c[0]); push(c[3]); push(c[1]); push(c[3]); push(c[2]);
        push(c[3]); push(c[2]); push(c[6]); push(c[3]); push(c[6]); push(c[7]);
        push(c[0]); push(c[1]); push(c[5]); push(c[0]); push(c[5]); push(c[4]);
        push(c[0]); push(c[3]); push(c[7]); push(c[0]); push(c[7]); push(c[4]);
        push(c[1]); push(c[2]); push(c[6]); push(c[1]); push(c[6]); push(c[5]);
    }

    glBindVertexArray(m_obstacleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_obstacleVBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / 3));
    glBindVertexArray(0);
}

void Renderer::setProjection(const glm::mat4& projection) {
    m_projection = projection;
}

void Renderer::setView(const glm::mat4& view) {
    m_view = view;
}

void Renderer::initGridBuffers() {
    glGenVertexArrays(1, &m_gridVAO);
    glGenBuffers(1, &m_gridVBO);

    glBindVertexArray(m_gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_gridVBO);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);

    glBindVertexArray(0);
}

void Renderer::renderGridWireframe(const Grid& grid) {
    if (!m_initialized) {
        return;
    }

    m_gridShader.use();
    m_gridShader.setMat4("uProjection", m_projection);
    m_gridShader.setMat4("uView", m_view);

    std::vector<glm::vec3> vertices = grid.getGridWireframeVertices();

    if (vertices.empty()) {
        return;
    }

    glBindVertexArray(m_gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_gridVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_DYNAMIC_DRAW);

    glLineWidth(1.0f);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices.size()));

    glBindVertexArray(0);
}

void Renderer::clear(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

std::string Renderer::findCampfirePath() {
    std::filesystem::path exePath = std::filesystem::current_path();
    std::filesystem::path bases[] = {
        exePath,
        exePath.parent_path(),
        exePath.parent_path().parent_path(),
        exePath.parent_path().parent_path().parent_path()
    };
    for (const auto& base : bases) {
        std::filesystem::path p = base / "Campfire" / "campfire.glb";
        try {
            if (std::filesystem::exists(p)) return std::filesystem::canonical(p).string();
        } catch (...) {}
    }
    return "";
}

bool Renderer::loadCampfire(const std::string& path) {
    return m_campfire.loadFromFile(path);
}

void Renderer::renderCampfire(const glm::vec3& position, bool wireframe) const {
    if (!m_initialized || !m_campfire.isLoaded()) return;
    if (wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    m_modelShader.use();
    m_modelShader.setMat4("uProjection", m_projection);
    m_modelShader.setMat4("uView", m_view);
    m_modelShader.setVec3("uLightDir", m_light.getDirection()); // przekaż kierunek światła
    glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
    model = model * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = model * glm::scale(glm::mat4(1.0f), glm::vec3(2.f));
    m_modelShader.setMat4("uModel", model);
    m_campfire.draw(&m_modelShader);
    if (wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
