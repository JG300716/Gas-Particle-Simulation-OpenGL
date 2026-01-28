#pragma once

#include "SmokeSimulation.h"
#include "../Render/Camera.h"
#include "../../ImGui/imgui.h"

class ImGuiControls {
public:
    static void renderAllControls(SmokeSimulation& simulation, Camera& camera);
    static void renderPerformanceOverlay(float deltaTime, float renderTime);
};
