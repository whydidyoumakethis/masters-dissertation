#include "DebugInterface.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <spdlog/spdlog.h>
#include <renderer/RenderManager.hpp>
#include <Components/DebugComponent.hpp>
#include <renderer/SceneManager.hpp>

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
                ImGui::Indent();
                if (ImGui::CollapsingHeader("Shaders")) {
                    ImGui::Text("Shader Path:");
                    ImGui::SameLine(125.0f);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                    ImGui::TextWrapped(PROJECT_SHADER_PATH);
                    ImGui::PopStyleColor();

                    ImGui::SeparatorText("pbr");
                    char pbr_v[256];
                    snprintf(pbr_v, sizeof(pbr_v), "%s", renderManager.shaderPaths.pbr_v.c_str());

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Vertex:");
                    ImGui::SameLine(125.0f);
                    if (ImGui::InputText("##editpbrv", pbr_v, sizeof(pbr_v), ImGuiInputTextFlags_EnterReturnsTrue)) {
                        renderManager.shaderPaths.pbr_v = pbr_v;
                    }

                    char pbr_f[256];
                    snprintf(pbr_f, sizeof(pbr_f), "%s", renderManager.shaderPaths.pbr_f.c_str());

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Fragment:");
                    ImGui::SameLine(125.0f);
                    if (ImGui::InputText("##editpbrf", pbr_f, sizeof(pbr_f), ImGuiInputTextFlags_EnterReturnsTrue)) {
                        renderManager.shaderPaths.pbr_f = pbr_f;
                    }

                    ImGui::SeparatorText("pbr_alpha");
                    char pbr_alpha_v[256];
                    snprintf(pbr_alpha_v, sizeof(pbr_alpha_v), "%s", renderManager.shaderPaths.pbr_alpha_v.c_str());

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Vertex:");
                    ImGui::SameLine(125.0f);
                    if (ImGui::InputText("##editpbr_alphav", pbr_alpha_v, sizeof(pbr_alpha_v), ImGuiInputTextFlags_EnterReturnsTrue)) {
                        renderManager.shaderPaths.pbr_alpha_v = pbr_alpha_v;
                    }

                    char pbr_alpha_f[256];
                    snprintf(pbr_alpha_f, sizeof(pbr_alpha_f), "%s", renderManager.shaderPaths.pbr_alpha_f.c_str());

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Fragment:");
                    ImGui::SameLine(125.0f);
                    if (ImGui::InputText("##editpbr_alphaf", pbr_alpha_f, sizeof(pbr_alpha_f), ImGuiInputTextFlags_EnterReturnsTrue)) {
                        renderManager.shaderPaths.pbr_alpha_f = pbr_alpha_f;
                    }

                    ImGui::SeparatorText("deferred_geometry");
                    char deferred_geometry_v[256];
                    snprintf(deferred_geometry_v, sizeof(deferred_geometry_v), "%s", renderManager.shaderPaths.deferred_geometry_v.c_str());

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Vertex:");
                    ImGui::SameLine(125.0f);
                    if (ImGui::InputText("##editdeferred_geometryv", deferred_geometry_v, sizeof(deferred_geometry_v), ImGuiInputTextFlags_EnterReturnsTrue)) {
                        renderManager.shaderPaths.deferred_geometry_v = deferred_geometry_v;
                    }

                    char deferred_geometry_f[256];
                    snprintf(deferred_geometry_f, sizeof(deferred_geometry_f), "%s", renderManager.shaderPaths.deferred_geometry_f.c_str());

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Fragment:");
                    ImGui::SameLine(125.0f);
                    if (ImGui::InputText("##editdeferred_geometryf", deferred_geometry_f, sizeof(deferred_geometry_f), ImGuiInputTextFlags_EnterReturnsTrue)) {
                        renderManager.shaderPaths.deferred_geometry_f = deferred_geometry_f;
                    }

                    ImGui::SeparatorText("deferred_geometry_alpha");
                    char deferred_geometry_alpha_v[256];
                    snprintf(deferred_geometry_alpha_v, sizeof(deferred_geometry_alpha_v), "%s", renderManager.shaderPaths.deferred_geometry_alpha_v.c_str());

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Vertex:");
                    ImGui::SameLine(125.0f);
                    if (ImGui::InputText("##editdeferred_geometry_alphav", deferred_geometry_alpha_v, sizeof(deferred_geometry_alpha_v), ImGuiInputTextFlags_EnterReturnsTrue)) {
                        renderManager.shaderPaths.deferred_geometry_alpha_v = deferred_geometry_alpha_v;
                    }

                    char deferred_geometry_alpha_f[256];
                    snprintf(deferred_geometry_alpha_f, sizeof(deferred_geometry_alpha_f), "%s", renderManager.shaderPaths.deferred_geometry_alpha_f.c_str());

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Fragment:");
                    ImGui::SameLine(125.0f);
                    if (ImGui::InputText("##editdeferred_geometry_alphaf", deferred_geometry_alpha_f, sizeof(deferred_geometry_alpha_f), ImGuiInputTextFlags_EnterReturnsTrue)) {
                        renderManager.shaderPaths.deferred_geometry_alpha_f = deferred_geometry_alpha_f;
                    }

                    ImGui::SeparatorText("deferred_lighting");
                    char deferred_lighting_v[256];
                    snprintf(deferred_lighting_v, sizeof(deferred_lighting_v), "%s", renderManager.shaderPaths.deferred_lighting_v.c_str());

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Vertex:");
                    ImGui::SameLine(125.0f);
                    if (ImGui::InputText("##editdeferred_lightingv", deferred_lighting_v, sizeof(deferred_lighting_v), ImGuiInputTextFlags_EnterReturnsTrue)) {
                        renderManager.shaderPaths.deferred_lighting_v = deferred_lighting_v;
                    }

                    char deferred_lighting_f[256];
                    snprintf(deferred_lighting_f, sizeof(deferred_lighting_f), "%s", renderManager.shaderPaths.deferred_lighting_f.c_str());

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Fragment:");
                    ImGui::SameLine(125.0f);
                    if (ImGui::InputText("##editdeferred_lightingf", deferred_lighting_f, sizeof(deferred_lighting_f), ImGuiInputTextFlags_EnterReturnsTrue)) {
                        renderManager.shaderPaths.deferred_lighting_f = deferred_lighting_f;
                    }

                    ImGui::Separator();

                    if (ImGui::Button("Reload Shaders", ImVec2(150.0f, 0.0f))) {
                        RenderManager::get().recreatePipelines();
                    }
                }

                if (ImGui::CollapsingHeader("Skybox")) {
                    static char right[512];
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Right:");
                    ImGui::SameLine(125.0f);
                    ImGui::InputText("##editskyboxright", right, sizeof(right));

                    static char left[512];
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Left:");
                    ImGui::SameLine(125.0f);
                    ImGui::InputText("##editskyboxleft", left, sizeof(left));

                    static char up[512];
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Up:");
                    ImGui::SameLine(125.0f);
                    ImGui::InputText("####editskyboxup", up, sizeof(up));

                    static char down[512];
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Down:");
                    ImGui::SameLine(125.0f);
                    ImGui::InputText("####editskyboxdown", down, sizeof(down));

                    static char front[512];
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Front:");
                    ImGui::SameLine(125.0f);
                    ImGui::InputText("####editskyboxfront", front, sizeof(front));

                    static char back[512];
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Back:");
                    ImGui::SameLine(125.0f);
                    ImGui::InputText("####editskyboxback", back, sizeof(back));

                    ImGui::Separator();

                    if (ImGui::Button("Create Skybox", ImVec2(150.0f, 0.0f))) {
                        RenderManager::get().setCustomSkybox(right, left, up, down, front, back);
                    }
                }
                ImGui::Unindent();
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
                    ImGui::SetTooltip("Whether or not the DebugCamera is allowed. You can toggle the camera at any time using F4.");
                }
                ImGui::SameLine(100.0f);
                ImGui::Checkbox("##debugcamallow", &debugCamAllowed);

                ImGui::Spacing();
                if (ImGui::Button("Reset"))
                    debugCam.reset();
            }

            if (ImGui::CollapsingHeader("Scene Settings")) {
                static bool useAssetPath = true;
                static bool loading = false;

                ImGui::AlignTextToFramePadding();
                ImGui::Text("Use Asset Path:");
                ImGui::SameLine();
                ImGui::TextDisabled("(?)");

                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip(PROJECT_ASSETS_PATH);
                }
                ImGui::SameLine();
                ImGui::Checkbox("##useassetpath", &useAssetPath);

                static char asset[512];
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Asset:");
                ImGui::SameLine(100.0f);
                ImGui::InputText("####asset", asset, sizeof(asset));

                ImGui::Indent(100.0f);
                ImGui::BeginDisabled(loading);
                if (ImGui::Button(loading ? "Loading...":"Load as model", ImVec2(100.0f, 0.0f))) {
                    loading = true;
                    std::filesystem::path path = useAssetPath ? std::filesystem::path(PROJECT_ASSETS_PATH) / asset:asset;

                    std::thread([path]() {
                        std::filesystem::path p = path;
                        SceneManager::get().loadModel(p.string());
                        loading = false;
                    }).detach();
                }

                ImGui::SameLine();

                if (ImGui::Button(loading ? "Loading...##2":"Load as scene", ImVec2(100.0f, 0.0f))) {
                    loading = true;
                    std::filesystem::path path = useAssetPath ? std::filesystem::path(PROJECT_ASSETS_PATH) / asset:asset;

                    std::thread([path]() {
                        std::filesystem::path p = path;
                        SceneManager::get().loadScene(Kiki::GltfLoaderAssimp::loadScene(p));
                        loading = false;
                    }).detach();
                }
                ImGui::EndDisabled();
                ImGui::Unindent(94.0f);

                ImGui::Separator();

                if (ImGui::Button("Clear Level", ImVec2(150.0f, 0.0f))) {
                    SceneManager::get().clearLevel();
                }
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