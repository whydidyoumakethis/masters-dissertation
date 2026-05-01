#include "physics/PhysicsDebugRenderer.hpp"

namespace Kiki {

    PhysicsDebugRenderer& PhysicsDebugRenderer::get() {
        static PhysicsDebugRenderer instance;
        return instance;
    }

    void PhysicsDebugRenderer::clear() {
        vertices.clear();
    }

    void PhysicsDebugRenderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) {
        const glm::vec4 colour(
            inColor.r / 255.0f,
            inColor.g / 255.0f,
            inColor.b / 255.0f,
            inColor.a / 255.0f
        );

        vertices.push_back({
            glm::vec3(static_cast<float>(inFrom.GetX()), static_cast<float>(inFrom.GetY()), static_cast<float>(inFrom.GetZ())),
            colour
            });
        vertices.push_back({
            glm::vec3(static_cast<float>(inTo.GetX()), static_cast<float>(inTo.GetY()), static_cast<float>(inTo.GetZ())),
            colour
            });
    }

    void PhysicsDebugRenderer::DrawText3D(JPH::RVec3Arg, const JPH::string_view&, JPH::ColorArg, float) {
        // Text labels can be routed to ImGui later; the wireframe pass only consumes lines.
    }

}