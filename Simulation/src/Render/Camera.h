#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    Camera();
    
    // Ustaw pozycję kamery
    void setPosition(const glm::vec3& position);
    [[nodiscard]] glm::vec3 getPosition() const { return m_position; }
    
    // Obracanie kamery
    void rotate(float yaw, float pitch); // w stopniach
    void addRotation(float deltaYaw, float deltaPitch);
    [[nodiscard]] float getYaw() const { return m_yaw; }
    [[nodiscard]] float getPitch() const { return m_pitch; }
    
    void setFOV(float fov) { m_fov = fov; }
    [[nodiscard]] float getFOV() const { return m_fov; }
    void setNearFar(float nearPlane, float farPlane);
    
    [[nodiscard]] glm::mat4 getViewMatrix() const;
    [[nodiscard]] glm::mat4 getProjectionMatrix(float aspectRatio) const;
    
    [[nodiscard]] glm::vec3 getForward() const;
    [[nodiscard]] glm::vec3 getRight() const;
    [[nodiscard]] glm::vec3 getUp() const;

private:
    glm::vec3 m_position;
    float m_yaw;      
    float m_pitch;
    
    float m_fov;
    float m_nearPlane;
    float m_farPlane;
    
    void updateVectors();
    glm::vec3 m_forward;
    glm::vec3 m_right;
    glm::vec3 m_up;
};
