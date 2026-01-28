#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    Camera();
    
    // Ustaw pozycję kamery
    void setPosition(const glm::vec3& position);
    glm::vec3 getPosition() const { return m_position; }
    
    // Obracanie kamery
    void rotate(float yaw, float pitch); // w stopniach
    void addRotation(float deltaYaw, float deltaPitch);
    
    // Ustaw kąt widzenia
    void setFOV(float fov) { m_fov = fov; }
    float getFOV() const { return m_fov; }
    
    // Ustaw zakresy renderowania
    void setNearFar(float nearPlane, float farPlane);
    
    // Pobierz macierz widoku
    glm::mat4 getViewMatrix() const;
    
    // Pobierz macierz projekcji
    glm::mat4 getProjectionMatrix(float aspectRatio) const;
    
    // Pobierz kierunek kamery
    glm::vec3 getForward() const;
    glm::vec3 getRight() const;
    glm::vec3 getUp() const;

private:
    glm::vec3 m_position;
    float m_yaw;      // Obrót wokół osi Y (lewo/prawo)
    float m_pitch;   // Obrót wokół osi X (góra/dół)
    
    float m_fov;
    float m_nearPlane;
    float m_farPlane;
    
    void updateVectors();
    glm::vec3 m_forward;
    glm::vec3 m_right;
    glm::vec3 m_up;
};
