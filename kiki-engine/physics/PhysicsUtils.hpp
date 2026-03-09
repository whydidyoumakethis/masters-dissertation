#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <Jolt/Jolt.h>

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
}