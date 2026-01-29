#pragma once

#include <string>
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

class Shader;

// Model GLTF/GLB – wiele części (wszystkie meshe/primitives), kolor z materiału.
class GltfModel {
public:
    GltfModel() = default;
    ~GltfModel();

    GltfModel(const GltfModel&) = delete;
    GltfModel& operator=(const GltfModel&) = delete;

    bool loadFromFile(const std::string& path);
    bool isLoaded() const { return m_loaded; }

    void draw() const;
    void draw(const Shader* shader) const;

private:
    void cleanup();

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 nrm;
        glm::vec2 uv;
    };

    struct Part {
        GLuint vao{ 0 };
        GLuint vbo{ 0 };
        GLuint ebo{ 0 };
        GLsizei indexCount{ 0 };
        glm::vec3 baseColor{ 0.6f, 0.35f, 0.2f };
    };

    bool m_loaded{ false };
    std::vector<Part> m_parts;
};
