#include "DebugInterface.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <spdlog/spdlog.h>
#include <renderer/RenderManager.hpp>

#include "windows/EntityViewer.hpp"


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


        ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);
        ImGui::Begin("kiki debugger");

        if (ImGui::Button(entityViewer ? "Close Entity Viewer":"Open Entity Viewer")) 
            entityViewer = !entityViewer;

        if (ImGui::CollapsingHeader("Performance")) {
            ImGui::Text("FPS: %.1f", io.Framerate);
            ImGui::Text("Frametime: %.3fms", io.DeltaTime * 1000.0f);
        }

        if (ImGui::CollapsingHeader("Render Settings")) {
            if (ImGui::Button("Reload Shaders")) {
                RenderManager::get().recreatePipelines();
            }
        }

        ImGui::End();

        if (entityViewer)
            debug::EntityViewer::draw(&entityViewer);


        // ImGui::ShowDemoWindow();
    }

    void DebugInterface::shutdown() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
}