#pragma once

#include <glad/glad.h>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader {
public:
    Shader();
    ~Shader();

    // Ładuje shader z kodu źródłowego
    bool loadFromSource(const std::string& vertexSource, const std::string& fragmentSource);
    
    // Ładuje shader z plików
    bool loadFromFile(const std::string& vertexPath, const std::string& fragmentPath);
    
    // Używa shadera
    void use() const;
    
    // Ustawia uniformy
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec4(const std::string& name, const glm::vec4& value) const;
    void setMat4(const std::string& name, const glm::mat4& value) const;

    GLuint getID() const { return m_programID; }

private:
    GLuint m_programID;
    
    // Kompiluje shader
    GLuint compileShader(GLenum type, const std::string& source);
    // Linkuje program
    bool linkProgram(GLuint vertexShader, GLuint fragmentShader);
    // Sprawdza błędy kompilacji
    bool checkCompileErrors(GLuint shader, const std::string& type);
};
