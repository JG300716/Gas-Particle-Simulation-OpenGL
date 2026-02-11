#pragma once

#include <glad/glad.h>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader {
public:
    Shader();
    ~Shader();

    bool loadFromSource(const std::string& vertexSource, const std::string& fragmentSource);
    bool loadFromFile(const std::string& vertexPath, const std::string& fragmentPath);
    void use() const;
    
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
    
    GLuint compileShader(GLenum type, const std::string& source);
    bool linkProgram(GLuint vertexShader, GLuint fragmentShader);
    bool checkCompileErrors(GLuint shader, const std::string& type);
};
