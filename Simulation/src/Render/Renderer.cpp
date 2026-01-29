#include "Renderer.h"
#include "../Simulation/Grid.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <filesystem>

Renderer::Renderer() : m_initialized(false), m_particleVAO(0), m_particleVBO(0), 
                       m_obstacleVAO(0), m_obstacleVBO(0), m_gridVAO(0), m_gridVBO(0) {
    m_projection = glm::mat4(1.0f);
    m_view = glm::mat4(1.0f);
}

Renderer::~Renderer() {
    if (m_particleVAO != 0) {
        glDeleteVertexArrays(1, &m_particleVAO);
    }
    if (m_particleVBO != 0) {
        glDeleteBuffers(1, &m_particleVBO);
    }
    if (m_obstacleVAO != 0) {
        glDeleteVertexArrays(1, &m_obstacleVAO);
    }
    if (m_obstacleVBO != 0) {
        glDeleteBuffers(1, &m_obstacleVBO);
    }
    if (m_gridVAO != 0) {
        glDeleteVertexArrays(1, &m_gridVAO);
    }
    if (m_gridVBO != 0) {
        glDeleteBuffers(1, &m_gridVBO);
    }
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
    
    // Załaduj shadery z plików
    std::string particleVert = shaderPath + "/particle.vert";
    std::string particleFrag = shaderPath + "/particle.frag";
    if (!m_particleShader.loadFromFile(particleVert, particleFrag)) {
        std::cerr << "Failed to load particle shader from: " << particleVert << ", " << particleFrag << std::endl;
        return false;
    }

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

    // Inicjalizuj buffery
    initParticleBuffers();
    initObstacleBuffers();
    initGridBuffers();

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

void Renderer::initParticleBuffers() {
    glGenVertexArrays(1, &m_particleVAO);
    glGenBuffers(1, &m_particleVBO);

    glBindVertexArray(m_particleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_particleVBO);
    
    // Pozycja (vec3), rozmiar (float), gęstość (float)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)0);
    
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(sizeof(float) * 3));
    
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(sizeof(float) * 4));

    glBindVertexArray(0);
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

void Renderer::renderGasParticles(const std::vector<GasParticle>& particles) {
    if (!m_initialized || particles.empty()) return;

    m_particleShader.use();
    m_particleShader.setMat4("uProjection", m_projection);
    m_particleShader.setMat4("uView", m_view);

    const size_t numFloats = particles.size() * 5;
    if (m_particleDataBuffer.size() < numFloats)
        m_particleDataBuffer.resize(numFloats);

    float* dst = m_particleDataBuffer.data();
    for (const auto& p : particles) {
        *dst++ = p.position.x;
        *dst++ = p.position.y;
        *dst++ = p.position.z;
        *dst++ = p.size;
        *dst++ = p.density;
    }

    glBindVertexArray(m_particleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_particleVBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(numFloats * sizeof(float)), m_particleDataBuffer.data(), GL_DYNAMIC_DRAW);

    glEnable(GL_PROGRAM_POINT_SIZE);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(particles.size()));
    glDisable(GL_PROGRAM_POINT_SIZE);

    glBindVertexArray(0);
}

void Renderer::renderObstacles(const std::vector<glm::vec4>& obstacles) {
    if (!m_initialized || obstacles.empty()) {
        return;
    }

    m_obstacleShader.use();
    m_obstacleShader.setMat4("uProjection", m_projection);
    m_obstacleShader.setMat4("uView", m_view);

    // Dla każdej przeszkody renderuj prostokąt (2D dla kompatybilności)
    for (const auto& obstacle : obstacles) {
        float x = obstacle.x;
        float y = obstacle.y;
        float w = obstacle.z;
        float h = obstacle.w;

        // Wierzchołki prostokąta (2D, z=0)
        float vertices[] = {
            x, y, 0.0f,
            x + w, y, 0.0f,
            x + w, y + h, 0.0f,
            x, y, 0.0f,
            x + w, y + h, 0.0f,
            x, y + h, 0.0f
        };

        glBindVertexArray(m_obstacleVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_obstacleVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

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

    // Pobierz wierzchołki grid'a
    std::vector<glm::vec3> vertices = grid.getGridWireframeVertices();

    if (vertices.empty()) {
        return;
    }

    glBindVertexArray(m_gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_gridVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_DYNAMIC_DRAW);

    // Renderuj jako linie
    glLineWidth(1.0f);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices.size()));

    glBindVertexArray(0);
}

GLuint Renderer::createComputeProgram(const char* compPath)
{
    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, &compPath, nullptr);
    glCompileShader(shader);

    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(shader, 2048, nullptr, log);
        throw std::runtime_error(log);
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, shader);
    glLinkProgram(prog);

    glDeleteShader(shader);
    return prog;
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

void Renderer::renderCampfire(const glm::vec3& position) const {
    if (!m_initialized || !m_campfire.isLoaded()) return;
    m_modelShader.use();
    m_modelShader.setMat4("uProjection", m_projection);
    m_modelShader.setMat4("uView", m_view);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
    model = model * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = model * glm::scale(glm::mat4(1.0f), glm::vec3(5.f));
    m_modelShader.setMat4("uModel", model);
    m_campfire.draw(&m_modelShader);
}
