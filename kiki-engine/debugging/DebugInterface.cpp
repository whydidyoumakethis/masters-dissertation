#include "DebugInterface.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <spdlog/spdlog.h>
#include <renderer/RenderManager.hpp>
#include <Components/DebugComponent.hpp>

#include "windows/Log.hpp"

namespace Kiki {
    DebugInterface& DebugInterface::get() {
        static DebugInterface instance;
        return instance;
    }
    
    void DebugInterface::initialise() {
        RenderManager& renderManager = Kiki::RenderManager::get();

        if (renderManager.isInitialised()) {
            if (!initialised) {
                initialised = true;
                const char* version = ImGui::GetVersion();
                spdlog::info("Loading ImGui version {}", version);

                ImGui::CreateContext();
                ImGuiIO& io = ImGui::GetIO();
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

                ImGui_ImplGlfw_InitForVulkan(Kiki::RenderManager::get().getWindow(), true);

                ImGui_ImplVulkan_InitInfo initInfo = {};
                renderManager.setDebugInterfaceInit(initInfo);
                ImGui_ImplVulkan_LoadFunctions(VK_API_VERSION_1_4, [](const char* function_name, void* user_data) {
                    return vkGetInstanceProcAddr(*(VkInstance*)user_data, function_name);
                }, &initInfo.Instance);

                ImGui_ImplVulkan_Init(&initInfo);

                // Set global style
                ImGuiStyle& style = ImGui::GetStyle();
                style.WindowBorderSize = 1.0f;
                style.ChildBorderSize = 1.0f;
                style.WindowRounding = 4.0f;
                style.ChildRounding = 4.0f;
                style.PopupRounding = 4.0f;
                style.FrameRounding = 2.0f;

                auto& world = World::Get();
                auto& registry = world.Registry();

                for (auto e : world.Query<entt::entity>()) {
                    registry.emplace<DebugComponent>(e);
                }
            } else {
                spdlog::warn("DebugInterface already initialised");
            }
        } else {
            spdlog::error("Attempted to initialise DebugInterface before RenderManager, DebugInterface not initialised");
        }
    }

    void DebugInterface::update(float dt) {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuiIO& io = ImGui::GetIO();

        if (InputManager::get().isKeyJustDown(GLFW_KEY_GRAVE_ACCENT))
            enabled = !enabled;

        if (InputManager::get().isKeyJustDown(GLFW_KEY_F4) && debugCamAllowed) {
            if (debugCam.enabled) {
                debugCam.enabled = false;
            } else {
                debugCam.enter();
            }
        }

        if (!debugCamAllowed)
            debugCam.enabled = false;

        if (enabled) {
            ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);
            ImGui::Begin("kiki debugger");

            ImGui::Indent(20.0f);
            ImGui::Dummy(ImVec2(0.0f, 6.0f));

            if (ImGui::Button(entityViewerVisible ? "Close Entity Viewer":"Open Entity Viewer", ImVec2(ImGui::GetContentRegionAvail().x - 20.0f, 0.0f))) 
                entityViewerVisible = !entityViewerVisible;

            if (ImGui::Button(logVisible ? "Close Log":"Open Log", ImVec2(ImGui::GetContentRegionAvail().x - 20.0f, 0.0f))) 
                logVisible = !logVisible;

            ImGui::Dummy(ImVec2(0.0f, 10.0f));
            ImGui::Unindent(20.0f);

            // Records frametime even if performance tab is closed
            static float frametimes[90];
            static int offset = 0;
            frametimes[offset] = io.DeltaTime * 1000.0f;
            offset = (offset + 1) % 90;

            if (ImGui::CollapsingHeader("Performance")) {
                ImGui::PlotHistogram("##Frametime", frametimes, 90, offset, nullptr, 0.0f, 50.0f, ImVec2(0.0f, 120.0f));

                ImGui::Spacing();
                ImGui::Text("FPS: %.1f", io.Framerate);
                float average = 0.0f;
                for (int i = 0; i < 90; i++)
                    average += frametimes[i];
                average /= 90;
                ImGui::Text("Frametime: %.3fms", average);
            }

            if (ImGui::CollapsingHeader("Render Settings")) {
                if (ImGui::Button("Reload Shaders")) {
                    RenderManager::get().recreatePipelines();
                }
            }

            if (ImGui::CollapsingHeader("Debug Camera Settings")) {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Speed:");
                ImGui::SameLine(100.0f);
                ImGui::InputFloat("##editcamspeed", &debugCam.speed, 0.0f, 0.0f, "%.1f");

                ImGui::AlignTextToFramePadding();
                ImGui::Text("Sensitivity:");
                ImGui::SameLine(100.0f);
                ImGui::InputFloat("##editcamsens", &debugCam.sensitivity, 0.0f, 0.0f, "%.3f");

                ImGui::AlignTextToFramePadding();
                ImGui::Text("Allowed:");
                ImGui::SameLine();
                ImGui::TextDisabled("(?)");

                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Whether or not the DebugCamera is allowed. You can toggle it at any time using F4");
                }
                ImGui::SameLine(100.0f);
                ImGui::Checkbox("##debugcamallow", &debugCamAllowed);

                ImGui::Spacing();
                if (ImGui::Button("Reset"))
                    debugCam.reset();
            }

            ImGui::End();

            if (entityViewerVisible)
                entityViewer.draw(&entityViewerVisible);
        }
    }

    void DebugInterface::shutdown() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
}