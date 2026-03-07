#pragma once
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <entt/entt.hpp>

namespace Kiki {

    class PhysicsContactListener : public JPH::ContactListener {
    public:
        // Triggered when two rigid bodies begin to come into contact
        virtual void OnContactAdded(
            const JPH::Body& inBody1,
            const JPH::Body& inBody2,
            const JPH::ContactManifold& inManifold,
            JPH::ContactSettings& ioSettings) override;

        // Triggered when two rigid bodies are in continuous contact (called every frame).
        virtual void OnContactPersisted(
            const JPH::Body& inBody1,
            const JPH::Body& inBody2,
            const JPH::ContactManifold& inManifold,
            JPH::ContactSettings& ioSettings) override;

        // Triggered when two rigid bodies separate.
        virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override;
    };
}