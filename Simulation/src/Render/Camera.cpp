#include "Camera.h"
#include <algorithm>

Camera::Camera() 
    : m_position(0.0f, 0.0f, 10.0f),
      m_yaw(-90.0f),
      m_pitch(0.0f),
      m_fov(45.0f),
      m_nearPlane(0.1f),
      m_farPlane(1000.0f) { // Zwiększony far plane dla większych grid'ów
    updateVectors();
}

void Camera::setPosition(const glm::vec3& position) {
    m_position = position;
}

void Camera::rotate(float yaw, float pitch) {
    m_yaw = yaw;
    m_pitch = pitch;
    
    // Ogranicz pitch do zakresu [-89, 89] stopni
    m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
    
    updateVectors();
}

void Camera::addRotation(float deltaYaw, float deltaPitch) {
    m_yaw += deltaYaw;
    m_pitch += deltaPitch;
    
    // Ogranicz pitch do zakresu [-89, 89] stopni
    m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
    
    updateVectors();
}

void Camera::setNearFar(float nearPlane, float farPlane) {
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(m_position, m_position + m_forward, m_up);
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(m_fov), aspectRatio, m_nearPlane, m_farPlane);
}

glm::vec3 Camera::getForward() const {
    return m_forward;
}

glm::vec3 Camera::getRight() const {
    return m_right;
}

glm::vec3 Camera::getUp() const {
    return m_up;
}

void Camera::updateVectors() {
    // Oblicz nowy wektor forward
    glm::vec3 forward;
    forward.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    forward.y = sin(glm::radians(m_pitch));
    forward.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_forward = glm::normalize(forward);
    
    // Oblicz wektor right i up
    m_right = glm::normalize(glm::cross(m_forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    m_up = glm::normalize(glm::cross(m_right, m_forward));
}
