#pragma once

#include "SmokeSimulation.h"
#include "../Render/Camera.h"
#include "../Render/Renderer.h"
#include "../../ImGui/imgui.h"

class Application;

class ImGuiControls {
public:
    static void renderAllControls(SmokeSimulation& simulation, Camera& camera, Renderer& renderer, Application* app = nullptr);
    static void renderPerformanceOverlay(float deltaTime, float renderTime);
};
