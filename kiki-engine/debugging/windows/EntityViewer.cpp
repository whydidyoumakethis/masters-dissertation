#include "EntityViewer.hpp"

#include "Components/DebugComponent.hpp"
#include "Components/ColourComponent.hpp"
#include "Components/CameraComponent.h"
#include "Components/MaterialComponent.hpp"
#include "Components/MeshComponent.hpp"
#include "Components/TransparencyComponent.hpp"
#include "ECS/GameObject.h"

#include <imgui.h>

#include <cstring>

namespace debug {
    EntityViewer& EntityViewer::get() {
        static EntityViewer instance;
        return instance;
    }

    void EntityViewer::draw(bool* visible) {
        ImGui::SetNextWindowSize(ImVec2(1080.0f, 480.0f));
        ImGui::Begin("Entity viewer", visible, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_MenuBar);

        if (ImGui::IsKeyPressed(ImGuiKey_Delete) && selectedEntity != entt::null)
            deleteSelected();

        // Menu bar
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Menu")) {
                if (ImGui::MenuItem("Clear all entities"))
                    destroyAllEntities();
                if (ImGui::MenuItem("Clear mesh entities"))
                    destroyMeshEntities();
                ImGui::Separator();
                if (ImGui::MenuItem("Close", NULL, false, visible != NULL))
                    *visible = false;
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Delete", "Del", false, selectedEntity != entt::null))
                    deleteSelected();
                if (ImGui::BeginMenu("Add component...", selectedEntity != entt::null)) {
                    if (ImGui::MenuItem("Tag"))
                        registry.emplace_or_replace<TagComponent>(selectedEntity);
                    if (ImGui::MenuItem("Transform"))
                        registry.emplace_or_replace<TransformComponent>(selectedEntity);
                    if (ImGui::MenuItem("Active"))
                        registry.emplace_or_replace<ActiveComponent>(selectedEntity);
                    if (ImGui::MenuItem("Camera"))
                        registry.emplace_or_replace<CameraComponent>(selectedEntity);
                    if (ImGui::MenuItem("Colour"))
                        registry.emplace_or_replace<ColourComponent>(selectedEntity);
                    if (ImGui::MenuItem("Mesh"))
                        registry.emplace_or_replace<MeshComponent>(selectedEntity);
                    if (ImGui::MenuItem("Material"))
                        registry.emplace_or_replace<MaterialComponent>(selectedEntity);
                    if (ImGui::MenuItem("Transparency"))
                        registry.emplace_or_replace<TransparencyComponent>(selectedEntity);
                    ImGui::EndMenu();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Create entity"))
                    world.CreateEntity();
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // Left side
        ImGui::BeginGroup();

        // Entities list
        auto t = world.Query<entt::entity>();
        std::vector<entt::entity> entities;

        for (auto e : t) {
            if (registry.valid(e)) {
                entities.push_back(e);

                if (!registry.all_of<DebugComponent>(e))
                    registry.emplace<DebugComponent>(e, true);
            }
        }

        int numEntities = entities.size();

        if (ImGui::BeginChild("Entities", ImVec2(722.0f, -(ImGui::GetTextLineHeightWithSpacing() + 6)), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoMove)) {
            int numRows = (numEntities + 9) / 10;
            ImGuiListClipper clipper;
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));

            clipper.Begin(numRows);
            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    for (int j = 0; j < 10; j++) {
                        int index = (i * 10) + j;

                        if (index >= numEntities) 
                            break; 

                        entt::entity e = entities[index];
                        auto id = std::to_string(entt::to_integral(e));
                        bool selected = selectedEntity == e;

                        ImGui::PushID(id.c_str());
                        ImVec2 pos = ImGui::GetCursorScreenPos();

                        // Add button
                        if (!selected) {
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 0.2f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.4f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 0.8f));
                        }

                        if (ImGui::Button("", ImVec2(62.0f, 62.0f))) {
                            if (selected) {
                                selectedEntity = entt::null;
                            } else {
                                selectedEntity = e;
                                world.GetComponent<DebugComponent>(e)->n = false;
                            }
                        }

                        if (!selected)
                            ImGui::PopStyleColor(3);

                        drawList->AddText(ImVec2(pos.x + 2, pos.y + 2), ImGui::GetColorU32(selected ? ImGuiCol_Text:ImGuiCol_TextDisabled), id.c_str());

                        if (world.GetComponent<DebugComponent>(e)->n)
                            drawList->AddCircleFilled(ImVec2(pos.x + 58.0f, pos.y + 4.0f), 2.0f, ImGui::GetColorU32(ImVec4(0.0f, 1.0f, 0.0f, 1.0f)));

                        if (registry.all_of<TagComponent>(e)) {
                            ImGui::PushClipRect(pos, ImVec2(pos.x + 62.0f, pos.y + 62.0f), true);
                            drawList->AddText(nullptr, 8.0f, ImVec2(pos.x + 2, pos.y + 52), ImGui::GetColorU32(ImGuiCol_TextDisabled), world.GetComponent<TagComponent>(e)->name.c_str());
                            ImGui::PopClipRect();
                        }

                        ImGui::PopID();

                        if (j < 9) 
                            ImGui::SameLine();
                    }
                }
            }

            ImGui::PopStyleVar();
        }
        ImGui::EndChild();

        // Total num of entities
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%d entities", numEntities);
        ImGui::SameLine(652.0f);
        if (ImGui::Button("Filter", ImVec2(70.0f, 0.0f))) {
            
        }

        ImGui::EndGroup();

        ImGui::SameLine();

        if (ImGui::BeginChild("Details", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoMove)) {
            ImGui::SetCursorPos(ImVec2(8.0f, 4.0f));
            ImGui::CollapsingHeader("Details", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
            
            if (selectedEntity != entt::null) {
                bool components = false; 
                float indent = (ImGui::GetContentRegionAvail().x - 100.0f) * 0.5f;

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.12f, 0.23f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.22f, 0.33f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.75f, 0.32f, 0.43f, 0.8f));

                if (registry.all_of<TagComponent>(selectedEntity)) {
                    components = true;
                    ImGui::SeparatorText("TagComponent");

                    auto* component = world.GetComponent<TagComponent>(selectedEntity);
                    char name[128];
                    snprintf(name, sizeof(name), "%s", component->name.c_str());

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Name:");
                    ImGui::SameLine(100.0f);
                    if (ImGui::InputText("##editname", name, sizeof(name), ImGuiInputTextFlags_EnterReturnsTrue)) {
                        component->name = name;
                    }

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    ImGui::Indent(indent);
                    if (ImGui::Button("Remove##tag", ImVec2(100.0f, 0.0f)))
                        registry.remove<TagComponent>(selectedEntity);
                    ImGui::Unindent(indent);
                }

                if (registry.all_of<TransformComponent>(selectedEntity)) {
                    components = true;
                    ImGui::SeparatorText("TransformComponent");

                    auto* component = world.GetComponent<TransformComponent>(selectedEntity);

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Position:");
                    ImGui::SameLine(100.0f);
                    ImGui::InputFloat3("##editpos", &component->position.x, "%.3f");

                    glm::vec3 rot = glm::degrees(glm::eulerAngles(component->rotation));

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Rotation:");
                    ImGui::SameLine(100.0f);
                    if (ImGui::InputFloat3("##editrot", &rot.x, "%.3f")) {
                        component->rotation = glm::quat(glm::radians(rot));
                    }

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Scale:");
                    ImGui::SameLine(100.0f);
                    ImGui::InputFloat3("##editscale", &component->scale.x, "%.3f");

                    component->dirty = true;

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    ImGui::Indent(indent);
                    if (ImGui::Button("Remove##transform", ImVec2(100.0f, 0.0f)))
                        registry.remove<TransformComponent>(selectedEntity);
                    ImGui::Unindent(indent);
                }

                if (registry.all_of<ActiveComponent>(selectedEntity)) {
                    components = true;
                    ImGui::SeparatorText("ActiveComponent");

                    auto* component = world.GetComponent<ActiveComponent>(selectedEntity);

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Active:");
                    ImGui::SameLine(100.0f);
                    ImGui::Checkbox("##editactive", &component->active);
                    
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    ImGui::Indent(indent);
                    if (ImGui::Button("Remove##active", ImVec2(100.0f, 0.0f)))
                        registry.remove<ActiveComponent>(selectedEntity);
                    ImGui::Unindent(indent);
                }

                if (registry.all_of<CameraComponent>(selectedEntity)) {
                    components = true;
                    ImGui::SeparatorText("CameraComponent");

                    auto* component = world.GetComponent<CameraComponent>(selectedEntity);

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("FOV:");
                    ImGui::SameLine(100.0f);
                    ImGui::SliderFloat("##editfov", &component->fov, 30.0f, 150.0f, "%.1f");

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Near:");
                    ImGui::SameLine(100.0f);
                    ImGui::InputFloat("##editnear", &component->nearPlane, 0.0f, 0.0f, "%.1f");

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Far:");
                    ImGui::SameLine(100.0f);
                    ImGui::InputFloat("##editfar", &component->farPlane, 0.0f, 0.0f, "%.1f");
                    
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    ImGui::Indent(indent);
                    if (ImGui::Button("Remove##camera", ImVec2(100.0f, 0.0f)))
                        registry.remove<CameraComponent>(selectedEntity);
                    ImGui::Unindent(indent);
                }

                if (registry.all_of<ColourComponent>(selectedEntity)) {
                    components = true;
                    ImGui::SeparatorText("ColourComponent");

                    auto* component = world.GetComponent<ColourComponent>(selectedEntity);

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Colour:");
                    ImGui::SameLine(100.0f);
                    ImGui::ColorEdit3("##editcolour", &component->colour.x);
                    
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    ImGui::Indent(indent);
                    if (ImGui::Button("Remove##colour", ImVec2(100.0f, 0.0f)))
                        registry.remove<ColourComponent>(selectedEntity);
                    ImGui::Unindent(indent);
                }

                if (registry.all_of<MeshComponent>(selectedEntity)) {
                    components = true;
                    ImGui::SeparatorText("MeshComponent");

                    auto* component = world.GetComponent<MeshComponent>(selectedEntity);

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Mesh ID:");
                    ImGui::SameLine(100.0f);
                    ImGui::InputInt("##editmesh", &component->id, 0, 0);
                    
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    ImGui::Indent(indent);
                    if (ImGui::Button("Remove##mesh", ImVec2(100.0f, 0.0f)))
                        registry.remove<MeshComponent>(selectedEntity);
                    ImGui::Unindent(indent);
                }

                if (registry.all_of<MaterialComponent>(selectedEntity)) {
                    components = true;
                    ImGui::SeparatorText("MaterialComponent");

                    auto* component = world.GetComponent<MaterialComponent>(selectedEntity);

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Material ID:");
                    ImGui::SameLine(100.0f);
                    ImGui::InputInt("##editmaterial", &component->id, 0, 0);
                    
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    ImGui::Indent(indent);
                    if (ImGui::Button("Remove##material", ImVec2(100.0f, 0.0f)))
                        registry.remove<MaterialComponent>(selectedEntity);
                    ImGui::Unindent(indent);
                }

                if (registry.all_of<TransparencyComponent>(selectedEntity)) {
                    components = true;
                    ImGui::SeparatorText("TransparencyComponent");

                    auto* component = world.GetComponent<TransparencyComponent>(selectedEntity);

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Transparency:");
                    ImGui::SameLine(100.0f);
                    ImGui::SliderFloat("##edittransparency", &component->transparency, 0.0f, 1.0f, "%.2f");

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Sprite:");
                    ImGui::SameLine(100.0f);
                    ImGui::Checkbox("##editsprite", &component->sprite);
                    
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    ImGui::Indent(indent);
                    if (ImGui::Button("Remove##transparency", ImVec2(100.0f, 0.0f)))
                        registry.remove<TransparencyComponent>(selectedEntity);
                    ImGui::Unindent(indent);
                }

                if (!components) {
                    ImVec2 size = ImGui::CalcTextSize("No (editable) components");
                    ImGui::Dummy(ImVec2(0.0f, 20.0f));
                    ImGui::Indent((ImGui::GetContentRegionAvail().x - size.x) * 0.5f);
                    ImGui::TextDisabled("No (editable) components");
                }

                ImGui::PopStyleColor(3);
            } else {
                ImVec2 size = ImGui::CalcTextSize("No entity selected");
                ImGui::Dummy(ImVec2(0.0f, 20.0f));
                ImGui::Indent((ImGui::GetContentRegionAvail().x - size.x) * 0.5f);
                ImGui::TextDisabled("No entity selected");
            }
        }
        ImGui::EndChild();
        

        ImGui::End();
    }

    void EntityViewer::deleteSelected() {
        world.DestroyEntity(selectedEntity);
        selectedEntity = entt::null;
    }

    void destroyAllEntities() {

    }

    void destroyMeshEntities() {

    }
}