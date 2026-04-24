#include "PhysicsContactListener.hpp"
#include <spdlog/spdlog.h>
#include <Jolt/Physics/Body/Body.h>

namespace Kiki {

    void PhysicsContactListener::OnContactAdded(
        const JPH::Body& inBody1, const JPH::Body& inBody2,
        const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
    {
        //Retrieve UserData (the Entity ID we stored when creating the rigid body)
        entt::entity e1 = (entt::entity)inBody1.GetUserData();
        entt::entity e2 = (entt::entity)inBody2.GetUserData();

        //A simple log test
        //spdlog::warn("[Collision] Entity {} hit Entity {}!", (uint32_t)e1, (uint32_t)e2);
		ms::Publish(CollisionEvent{ e1, e2 });
    }

    void PhysicsContactListener::OnContactPersisted(const JPH::Body&, const JPH::Body&, const JPH::ContactManifold&, JPH::ContactSettings&) {}

    void PhysicsContactListener::OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) {
    }
}