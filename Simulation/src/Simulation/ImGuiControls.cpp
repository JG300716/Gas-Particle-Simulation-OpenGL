#include "ImGuiControls.h"
#include <iostream>

void ImGuiControls::renderAllControls(SmokeSimulation& simulation, Camera& camera) {
    ImGui::Begin("Controls");
    ImGui::SetWindowPos(ImVec2(5, 5), ImGuiCond_FirstUseEver);
    
    // Grid size
    static bool gridSizeInitialized = false;
    static int gridSizeX = 20;
    static int gridSizeY = 20;
    static int gridSizeZ = 20;
    
    if (!gridSizeInitialized) {
        gridSizeX = simulation.getGridSizeX();
        gridSizeY = simulation.getGridSizeY();
        gridSizeZ = simulation.getGridSizeZ();
        gridSizeInitialized = true;
    }
    
    if (gridSizeX != simulation.getGridSizeX() || 
        gridSizeY != simulation.getGridSizeY() || 
        gridSizeZ != simulation.getGridSizeZ()) {
        gridSizeX = simulation.getGridSizeX();
        gridSizeY = simulation.getGridSizeY();
        gridSizeZ = simulation.getGridSizeZ();
    }
    
    int gridSizeArray[3] = { gridSizeX, gridSizeY, gridSizeZ };
    if (ImGui::InputInt3("Grid Size", gridSizeArray)) {
        gridSizeX = std::max(1, gridSizeArray[0]);
        gridSizeY = std::max(1, gridSizeArray[1]);
        gridSizeZ = std::max(1, gridSizeArray[2]);
        simulation.setGridSize(gridSizeX, gridSizeY, gridSizeZ);
    }
    
    // Ambient pressure
    float ambientPressure = simulation.getAmbientPressure();
    if (ImGui::SliderFloat("[hPa]", &ambientPressure, 900.0f, 1100.0f)) {
        simulation.setAmbientPressure(ambientPressure);
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset##Pressure")) {
        simulation.setAmbientPressure(1013.25f);
    }
    
    // Particle density
    float particleDensity = simulation.getParticleDensity();
    if (ImGui::SliderFloat("[g/cm^3]", &particleDensity, 0.01f, 2.0f, "%.6f")) {
        simulation.setParticleDensity(particleDensity);
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset##Density")) {
        simulation.setParticleDensity(1.0f);
    }
    
    // Gravity
    float gravity = simulation.getGravity();
    if (ImGui::SliderFloat("[m/s^2]", &gravity, -20.0f, 20.0f)) {
        simulation.setGravity(gravity);
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset##Gravity")) {
        simulation.setGravity(-9.81f);
    }
    float dampingFactor = simulation.getDampingFactor();
    if (ImGui::SliderFloat("Damping", &dampingFactor, 0.0f, 1.0f)) {
        simulation.setDampingFactor(dampingFactor);
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset##Damping")) {
        simulation.setDampingFactor(0.5f);
    }

    // Cell size
    float cellSize = simulation.getCellSize();
    if (ImGui::SliderFloat("Cell Size", &cellSize, 0.01f, 5.0f)) {
        simulation.setCellSize(cellSize);
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset##CellSize")) {
        simulation.setCellSize(1.0f);
    }
    
    // Time scale
    ImGui::Separator();
    float timeScale = simulation.getTimeScale();
    if (ImGui::SliderFloat("Time Scale", &timeScale, 0.0f, 5.0f)) {
        simulation.setTimeScale(timeScale);
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset##TimeScale")) {
        simulation.setTimeScale(1.0f);
    }
    
    if (ImGui::Button("Pause")) {
        simulation.setTimeScale(0.0f);
    }
    ImGui::SameLine();
    if (ImGui::Button("0.5x")) {
        simulation.setTimeScale(0.5f);
    }
    ImGui::SameLine();
    if (ImGui::Button("1.0x")) {
        simulation.setTimeScale(1.0f);
    }
    ImGui::SameLine();
    if (ImGui::Button("2.0x")) {
        simulation.setTimeScale(2.0f);
    }
    ImGui::SameLine();
    if (ImGui::Button("5.0x")) {
        simulation.setTimeScale(5.0f);
    }
    
    // Particle spawner
    ImGui::Separator();
    glm::vec3 spawnerPos = simulation.getSpawnerPosition();
    float spawnerPosArray[3] = { spawnerPos.x, spawnerPos.y, spawnerPos.z };
    if (ImGui::InputFloat3("Spawner", spawnerPosArray)) {
        simulation.setSpawnerPosition(glm::vec3(spawnerPosArray[0], spawnerPosArray[1], spawnerPosArray[2]));
    }
    static int spawnCount = 10;
    ImGui::InputInt("Count", &spawnCount);
    if (ImGui::Button("Spawn")) {
        simulation.spawnParticles(spawnCount);
    }
    
    // Obstacles
    ImGui::Separator();
    static float obstacleX = 0.0f;
    static float obstacleY = 0.0f;
    static float obstacleZ = 0.0f;
    static float obstacleWidth = 2.0f;
    static float obstacleHeight = 2.0f;
    static float obstacleDepth = 2.0f;
    
    float obstaclePosArray[3] = { obstacleX, obstacleY, obstacleZ };
    if (ImGui::InputFloat3("Obstacle Pos", obstaclePosArray)) {
        obstacleX = obstaclePosArray[0];
        obstacleY = obstaclePosArray[1];
        obstacleZ = obstaclePosArray[2];
    }
    
    float obstacleSizeArray[3] = { obstacleWidth, obstacleHeight, obstacleDepth };
    if (ImGui::InputFloat3("Obstacle Size", obstacleSizeArray)) {
        obstacleWidth = obstacleSizeArray[0];
        obstacleHeight = obstacleSizeArray[1];
        obstacleDepth = obstacleSizeArray[2];
    }
    
    if (ImGui::Button("Add Obstacle")) {
        Obstacle obstacle;
        obstacle.position = glm::vec3(obstacleX, obstacleY, obstacleZ);
        obstacle.size = glm::vec3(obstacleWidth, obstacleHeight, obstacleDepth);
        simulation.addObstacle(obstacle);
    }
    
    const auto& obstacles = simulation.getObstacles();
    if (!obstacles.empty()) {
        for (size_t i = 0; i < obstacles.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));
            ImGui::Text("%zu: (%.1f,%.1f,%.1f)", i, obstacles[i].position.x, obstacles[i].position.y, obstacles[i].position.z);
            ImGui::SameLine();
            if (ImGui::Button("X")) {
                simulation.removeObstacle(i);
            }
            ImGui::PopID();
        }
        if (ImGui::Button("Clear")) {
            simulation.clearObstacles();
        }
    }
    
    // Camera
    ImGui::Separator();
    glm::vec3 pos = camera.getPosition();
    float posArray[3] = { pos.x, pos.y, pos.z };
    if (ImGui::InputFloat3("Camera", posArray)) {
        camera.setPosition(glm::vec3(posArray[0], posArray[1], posArray[2]));
    }
    
    static float yaw = 0.0f;
    static float pitch = 0.0f;
    if (ImGui::SliderFloat("Yaw", &yaw, -180.0f, 180.0f)) {
        camera.rotate(yaw, pitch);
    }
    if (ImGui::SliderFloat("Pitch", &pitch, -89.0f, 89.0f)) {
        camera.rotate(yaw, pitch);
    }
    
    if (ImGui::Button("Reset Camera")) {
        camera.setPosition(glm::vec3(0.0f, 0.0f, 10.0f));
        yaw = -90.0f;
        pitch = 0.0f;
        camera.rotate(yaw, pitch);
    }
    
    // Statistics
    ImGui::Separator();
    ImGui::Text("Particles: %zu", simulation.getGasParticles().size());
    ImGui::Text("Obstacles: %zu", obstacles.size());
    
    if (ImGui::Button("Add Particles")) {
        simulation.addParticlesAt(glm::vec3(0.0f, 0.0f, 0.0f), 50);
    }
    
    ImGui::End();
}

void ImGuiControls::renderPerformanceOverlay(float deltaTime, float renderTime) {
    // Ustaw pozycję okna w prawym górnym rogu
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 220.0f, 10.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(210.0f, 0.0f), ImGuiCond_Always);
    
    // Styl okna bez ramki
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
    
    ImGui::Begin("Performance", nullptr, 
                 ImGuiWindowFlags_NoTitleBar | 
                 ImGuiWindowFlags_NoResize | 
                 ImGuiWindowFlags_NoMove | 
                 ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_AlwaysAutoResize);
    
    // FPS
    float fps = (deltaTime > 0.0f) ? (1000.0f / deltaTime) : 0.0f;
    ImGui::Text("FPS: %.1f", fps);
    
    // Czas renderowania
    ImGui::Text("Frame: %.2f ms", deltaTime);
    ImGui::Text("Render: %.2f ms", renderTime);
    
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
    ImGui::End();
}
