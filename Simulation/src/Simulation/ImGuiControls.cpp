#include "ImGuiControls.h"
#include "../Application/Application.h"
#include <iostream>
#include <string>
#include <cmath>

void ImGuiControls::renderAllControls(SmokeSimulation& simulation, Camera& camera, Renderer& renderer, Application* app) {
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
    // Parametry wstrzykiwania
    ImGui::Text("Smoke Injection:");
    float injRate = simulation.getInjectRate();
    if (ImGui::SliderFloat("Inject rate", &injRate, 0.f, 30.f)) simulation.setInjectRate(injRate);
    float injVel = simulation.getInjectVelocity();
    if (ImGui::SliderFloat("Inject velocity", &injVel, 0.f, 15.f)) simulation.setInjectVelocity(injVel);
    bool injectCyl = simulation.getInjectCylinder();
    if (ImGui::Checkbox("Cylinder shape", &injectCyl)) simulation.setInjectCylinder(injectCyl);
    
    if (ImGui::Button("Clear Smoke")) {
        simulation.clearSmoke();
    }
    
    // Parametry termiczne
    ImGui::Separator();
    ImGui::Text("Thermal:");
    float Tamb = simulation.getTempAmbient();
    if (ImGui::SliderFloat("T ambient", &Tamb, -50.f, 100.f)) simulation.setTempAmbient(Tamb);
    float tcool = simulation.getTempCooling();
    if (ImGui::SliderFloat("Cooling rate", &tcool, 0.f, 2.f)) simulation.setTempCooling(tcool);
    float rhoAir = simulation.getAirDensity();
    if (ImGui::InputFloat("Air density [g/m³]", &rhoAir, 10.f, 50.f)) simulation.setAirDensity(rhoAir);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("G\u0119sto\u015b\u0107 powietrza (np. ~1200)");
    float rhoSmoke = simulation.getSmokeParticleDensity();
    if (ImGui::InputFloat("Smoke density [g/m³]", &rhoSmoke, 10.f, 50.f)) simulation.setSmokeParticleDensity(rhoSmoke);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("G\u0119sto\u015b\u0107 cz\u0105steczek dymu; mniejsza ni\u017c powietrze = unosi si\u0119");
    float ba = simulation.getBuoyancyAlpha();
    if (ImGui::SliderFloat("Buoyancy scale", &ba, 0.f, 2.0f)) simulation.setBuoyancyAlpha(ba);
    float bb = simulation.getBuoyancyBeta();
    if (ImGui::SliderFloat("Thermal lift", &bb, 0.f, 3.f)) simulation.setBuoyancyBeta(bb);
    
    // Parametry ruchu
    ImGui::Separator();
    ImGui::Text("Motion:");
    float diff = simulation.getDiffusion();
    if (ImGui::SliderFloat("Diffusion", &diff, 0.f, 1.f)) simulation.setDiffusion(diff);
    float dissip = simulation.getDissipation();
    if (ImGui::SliderFloat("Dissipation", &dissip, 0.f, 0.5f)) simulation.setDissipation(dissip);
    float velDissip = simulation.getVelocityDissipation();
    if (ImGui::SliderFloat("Velocity drag", &velDissip, 0.f, 1.5f)) simulation.setVelocityDissipation(velDissip);
    float turb = simulation.getTurbulence();
    if (ImGui::SliderFloat("Turbulence", &turb, 0.f, 2.f)) simulation.setTurbulence(turb);
    float smallTurb = simulation.getSmallScaleTurbulenceGain();
    if (ImGui::SliderFloat("Small-scale turbulence", &smallTurb, 0.f, 2.f)) simulation.setSmallScaleTurbulenceGain(smallTurb);
    float vort = simulation.getVorticity();
    if (ImGui::SliderFloat("Vorticity", &vort, 0.f, 3.f)) simulation.setVorticity(vort);

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
    
    // === LIGHTING ===
    ImGui::Separator();
    ImGui::Text("Lighting");
    LightSettings& light = renderer.getLightSettings();
    
    ImGui::SliderFloat("Light Yaw", &light.yaw, -180.0f, 180.0f, "%.1f deg");
    ImGui::SliderFloat("Light Pitch", &light.pitch, -89.0f, 89.0f, "%.1f deg");
    ImGui::SliderFloat("Light Distance", &light.distance, 5.0f, 50.0f, "%.1f");
    ImGui::Checkbox("Show Light Indicator", &light.showIndicator);
    
    // Pokaż obliczony kierunek światła
    glm::vec3 lightDir = light.getDirection();
    ImGui::Text("Direction: (%.2f, %.2f, %.2f)", lightDir.x, lightDir.y, lightDir.z);
    glm::vec3 lightPos = light.getPosition();
    ImGui::Text("Position: (%.1f, %.1f, %.1f)", lightPos.x, lightPos.y, lightPos.z);
    
    if (ImGui::Button("Reset Light")) {
        light.yaw = 45.0f;
        light.pitch = 60.0f;
        light.distance = 15.0f;
    }
    
    // Camera
    ImGui::Separator();
    glm::vec3 pos = camera.getPosition();
    float posArray[3] = { pos.x, pos.y, pos.z };
    if (ImGui::InputFloat3("Camera", posArray)) {
        camera.setPosition(glm::vec3(posArray[0], posArray[1], posArray[2]));
    }
    
    static float yaw = -90.0f;
    static float pitch = -15.0f;
    // Synchronizuj suwaki z kamerą (po obrocie myszką wartości się zgadzają)
    float camYaw = camera.getYaw();
    float camPitch = camera.getPitch();
    if (std::abs(yaw - camYaw) > 0.5f || std::abs(pitch - camPitch) > 0.5f) {
        yaw = camYaw;
        pitch = camPitch;
    }
    if (ImGui::SliderFloat("Cam Yaw", &yaw, -180.0f, 180.0f)) {
        camera.rotate(yaw, pitch);
    }
    if (ImGui::SliderFloat("Cam Pitch", &pitch, -89.0f, 89.0f)) {
        camera.rotate(yaw, pitch);
    }

    if (ImGui::Button("Reset Camera")) {
        // Widok z +Z, Y w górę: dym leci w górę na ekranie
        camera.setPosition(glm::vec3(0.0f, 8.0f, 25.0f));
        yaw = -90.0f;
        pitch = -15.0f;
        camera.rotate(yaw, pitch);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Widok: Z prz\u00f3d, Y w g\u00f3r\u0119 (dym do g\u00f3ry)");
    }
    
    // Maksymalny refresh rate
    if (app) {
        ImGui::Separator();
        ImGui::Text("Refresh rate");
        int maxFps = app->getMaxFps();
        if (ImGui::InputInt("Max FPS", &maxFps, 1, 10)) {
            app->setMaxFps(std::max(0, maxFps));
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("0 = bez limitu");
        }
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
