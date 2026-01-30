#include "ImGuiControls.h"
#include <iostream>
#include <string>

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
    
    // Gravity
    float gravity = simulation.getGravity();
    if (ImGui::SliderFloat("[m/s^2]", &gravity, -20.0f, 20.0f)) {
        simulation.setGravity(gravity);
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset##Gravity")) {
        simulation.setGravity(-9.81f);
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
    
    // Źródło dymu (spawner / ognisko)
    ImGui::Separator();
    glm::vec3 spawnerPos = simulation.getSpawnerPosition();
    float spawnerPosArray[3] = { spawnerPos.x, spawnerPos.y, spawnerPos.z };
    if (ImGui::InputFloat3("Spawner (dym)", spawnerPosArray)) {
        simulation.setSpawnerPosition(glm::vec3(spawnerPosArray[0], spawnerPosArray[1], spawnerPosArray[2]));
    }
    float Tamb = simulation.getTempAmbient();
    if (ImGui::SliderFloat("T ambient (room)", &Tamb, -50.f, 100.f)) simulation.setTempAmbient(Tamb);
    float tcool = simulation.getTempCooling();
    if (ImGui::SliderFloat("Temp cooling/tick", &tcool, 0.f, 2.f)) simulation.setTempCooling(tcool);
    float diff = simulation.getDiffusion();
    if (ImGui::SliderFloat("Diffusion", &diff, 0.f, 10.f)) simulation.setDiffusion(diff);
    float injRate = simulation.getInjectRate();
    if (ImGui::SliderFloat("Inject rate", &injRate, 0.f, 50.f)) simulation.setInjectRate(injRate);
    bool injectCyl = simulation.getInjectCylinder();
    if (ImGui::Checkbox("Inject cylinder (else sphere)", &injectCyl)) simulation.setInjectCylinder(injectCyl);
    float ba = simulation.getBuoyancyAlpha();
    if (ImGui::SliderFloat("Buoyancy alpha (smoke weight)", &ba, 0.f, 0.5f)) simulation.setBuoyancyAlpha(ba);
    float bb = simulation.getBuoyancyBeta();
    if (ImGui::SliderFloat("Buoyancy beta (temp lift)", &bb, 0.f, 2.f)) simulation.setBuoyancyBeta(bb);

    // Sufit z dziurą
    ImGui::Separator();
    ImGui::Text("Ceiling (wall with hole)");
    WallWithHole& ceiling = simulation.getCeiling();
    bool ceilEnabled = ceiling.enabled;
    if (ImGui::Checkbox("Ceiling enabled", &ceilEnabled)) {
        simulation.setCeilingEnabled(ceilEnabled);
    }
    if (ceiling.enabled) {
        float holeCenter[2] = { ceiling.holeCenter.x, ceiling.holeCenter.y };
        if (ImGui::SliderFloat2("Hole center (X,Z)", holeCenter, -ceiling.width * 0.5f, ceiling.width * 0.5f)) {
            simulation.setCeilingHole(glm::vec2(holeCenter[0], holeCenter[1]), ceiling.holeSize);
        }
        float holeSize[2] = { ceiling.holeSize.x, ceiling.holeSize.y };
        if (ImGui::SliderFloat2("Hole size (W,D)", holeSize, 0.f, std::min(ceiling.width, ceiling.depth))) {
            simulation.setCeilingHole(ceiling.holeCenter, glm::vec2(holeSize[0], holeSize[1]));
        }
        float ceilY = ceiling.position.y;
        if (ImGui::SliderFloat("Ceiling Y", &ceilY, simulation.getGrid().getMinBounds().y, simulation.getGrid().getMaxBounds().y)) {
            ceiling.position.y = ceilY;
            simulation.updateCeiling();
        }
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
        obstacle.rotation = glm::vec3(0.f, 0.f, 0.f);
        obstacle.scale = glm::vec3(1.f, 1.f, 1.f);
        simulation.addObstacle(obstacle);
    }

    static int selectedObstacle = -1;
    const auto& obstacles = simulation.getObstacles();
    if (!obstacles.empty()) {
        for (size_t i = 0; i < obstacles.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));
            bool selected = (selectedObstacle == static_cast<int>(i));
            if (ImGui::Selectable(("Obstacle " + std::to_string(i) + "##sel").c_str(), selected))
                selectedObstacle = selected ? -1 : static_cast<int>(i);
            ImGui::SameLine();
            if (ImGui::Button("X")) {
                if (static_cast<int>(i) == selectedObstacle) selectedObstacle = -1;
                else if (i < static_cast<size_t>(selectedObstacle)) --selectedObstacle;
                simulation.removeObstacle(i);
            }
            ImGui::PopID();
        }
        if (selectedObstacle >= 0 && selectedObstacle < static_cast<int>(obstacles.size())) {
            Obstacle o = obstacles[selectedObstacle];
            float pos[3] = { o.position.x, o.position.y, o.position.z };
            if (ImGui::InputFloat3("Transform Position", pos)) {
                o.position = glm::vec3(pos[0], pos[1], pos[2]);
                simulation.updateObstacle(selectedObstacle, o);
            }
            float rot[3] = { o.rotation.x, o.rotation.y, o.rotation.z };
            if (ImGui::InputFloat3("Transform Rotation (deg)", rot)) {
                o.rotation = glm::vec3(rot[0], rot[1], rot[2]);
                simulation.updateObstacle(selectedObstacle, o);
            }
            float scl[3] = { o.scale.x, o.scale.y, o.scale.z };
            if (ImGui::InputFloat3("Transform Scale", scl)) {
                o.scale = glm::vec3(std::max(0.01f, scl[0]), std::max(0.01f, scl[1]), std::max(0.01f, scl[2]));
                simulation.updateObstacle(selectedObstacle, o);
            }
            float sz[3] = { o.size.x, o.size.y, o.size.z };
            if (ImGui::InputFloat3("Size (half-extents)", sz)) {
                o.size = glm::vec3(std::max(0.01f, sz[0]), std::max(0.01f, sz[1]), std::max(0.01f, sz[2]));
                simulation.updateObstacle(selectedObstacle, o);
            }
        }
        if (ImGui::Button("Clear")) {
            selectedObstacle = -1;
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
    ImGui::Text("Cz\u0105steczki: %d", simulation.getSmokeCellCount());
    ImGui::Text("Grid: %d x %d x %d", simulation.getGridSizeX(), simulation.getGridSizeY(), simulation.getGridSizeZ());
    ImGui::Text("Obstacles: %zu", obstacles.size());

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
