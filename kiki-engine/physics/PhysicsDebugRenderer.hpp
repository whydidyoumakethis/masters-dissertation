#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Renderer/DebugRendererSimple.h>
#include <glm/glm.hpp>

#include <vector>

namespace Kiki {

    struct DebugLineVertex {
        glm::vec3 position;
        glm::vec4 colour;
    };

    class PhysicsDebugRenderer : public JPH::DebugRendererSimple {
    public:
        static PhysicsDebugRenderer& get();

        void clear();
        void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;
        void DrawText3D(JPH::RVec3Arg inPosition, const JPH::string_view& inString, JPH::ColorArg inColor, float inHeight) override;
        const std::vector<DebugLineVertex>& getVertices() const { return vertices; }

        bool enabled = false;
        bool drawBodies = true;
        bool drawBoundingBoxes = false;
        bool drawVelocity = false;
    private:
        PhysicsDebugRenderer() = default;

        std::vector<DebugLineVertex> vertices;
    };
}