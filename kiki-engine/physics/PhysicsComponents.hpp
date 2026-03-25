#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <glm/glm.hpp>
#include <Jolt/Physics/Collision/Shape/Shape.h> 
#include <Jolt/Core/Reference.h>   

namespace Kiki {

    struct RigidBodyComponent {

        JPH::BodyID bodyID;

        // Static, Kinematic, Dynamic
        JPH::EMotionType motionType = JPH::EMotionType::Dynamic;

        // The logical layer to which the object belongs (used for collision filtering)
        uint16_t layer = 1; // default MOVING

        float friction = 0.5f;    // Friction: Typically 0.0 (ice surface) to 1.0 (rough surface)
        float restitution = 0.0f; // Elasticity: 0.0 (no elasticity) to 1.0 (fully elastic collision)
        bool isSensor = false;

        RigidBodyComponent(JPH::EMotionType type = JPH::EMotionType::Dynamic,
            uint16_t ly = 1,
            float res = 0.0f,
            float fri = 0.5f,
            bool sensor = false)
            : motionType(type), layer(ly), restitution(res), friction(fri), isSensor(sensor) {
        };
    };

    struct SphereColliderComponent {
        float radius = 0.5f;
    };

    struct BoxColliderComponent {
        glm::vec3 halfExtents = { 0.5f, 0.5f, 0.5f };
    };

    struct MeshColliderComponent {
        JPH::Ref<JPH::Shape> shape;
    };
    //......
    //Other shapes of colliders may also be needed, such as capsules.
}