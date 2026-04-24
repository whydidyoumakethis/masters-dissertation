#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <Jolt/Jolt.h>

#include <vector>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>

namespace Kiki {

    // --- GLM to Jolt ---

    inline JPH::Vec3 ToJPH(const glm::vec3& v) {
        return JPH::Vec3(v.x, v.y, v.z);
    }

    // to differ RVec3 & Vec3
#ifdef JPH_DOUBLE_PRECISION
    inline JPH::RVec3 ToJPHR(const glm::vec3& v) {
        return JPH::RVec3(v.x, v.y, v.z);
    }
#else
    inline JPH::Vec3 ToJPHR(const glm::vec3& v) {
        return JPH::Vec3(v.x, v.y, v.z);
    }
#endif

    inline JPH::Quat ToJPH(const glm::quat& q) {
        return JPH::Quat(q.x, q.y, q.z, q.w);
    }

    // --- Jolt to GLM ---

    inline glm::vec3 ToGLM(const JPH::Vec3& v) {
        return glm::vec3(v.GetX(), v.GetY(), v.GetZ());
    }

#ifdef JPH_DOUBLE_PRECISION
    inline glm::vec3 ToGLM(const JPH::RVec3& v) {
        return glm::vec3((float)v.GetX(), (float)v.GetY(), (float)v.GetZ());
    }
#endif

    inline glm::quat ToGLM(const JPH::Quat& q) {
        return glm::quat(q.GetW(), q.GetX(), q.GetY(), q.GetZ());
    }

    //for static colliders
    static JPH::Ref<JPH::Shape> CreateTriangleMesh(const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices) {
        JPH::VertexList joltVertices;
        joltVertices.reserve(vertices.size());
        for (const auto& v : vertices) {
            joltVertices.push_back(JPH::Float3(v.x, v.y, v.z));
        }

        JPH::IndexedTriangleList joltIndices;
        joltIndices.reserve(indices.size() / 3);
        for (size_t i = 0; i < indices.size(); i += 3) {
			//keep in range check
            joltIndices.push_back(JPH::IndexedTriangle(indices[i], indices[i + 1], indices[i + 2]));
        }

        JPH::MeshShapeSettings settings(joltVertices, joltIndices);
        JPH::Shape::ShapeResult result = settings.Create();

        return result.IsValid() ? result.Get() : nullptr;
    }

	// ConvexHull for dynamic colliders, but we still not using it in the current version.
    static JPH::Ref<JPH::Shape> CreateConvexHull(const std::vector<glm::vec3>& vertices) {
        JPH::ConvexHullShapeSettings settings;
        for (const auto& v : vertices) {
            settings.mPoints.push_back(ToJPH(v));
        }

        settings.mMaxConvexRadius = 0.05f;

        JPH::Shape::ShapeResult result = settings.Create();
        return result.IsValid() ? result.Get() : nullptr;
    }
}