#include "EntityViewer.hpp"

#include <imgui.h>

namespace debug {
    void EntityViewer::draw(bool* visible) {
        ImGui::SetNextWindowSize(ImVec2(iconSize * 25, iconSize * 15));
        ImGui::Begin("Entity viewer", visible, ImGuiWindowFlags_NoResize);

        ImGui::Text("Hi");
        ImGui::End();
    }
}