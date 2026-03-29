#pragma once

#include "ECS/System.h"
#include "physics/PhysicsManager.hpp"
#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace Kiki {

    struct RaycastHit {
        bool hasHit = false;
        entt::entity entity = entt::null;
        glm::vec3 position{ 0.0f };
        glm::vec3 normal{ 0.0f };
        float distance = 0.0f;
    };

    class PhysicsSystem : public System {
    public:
        PhysicsSystem() = default;
        ~PhysicsSystem() override = default;

        Phase GetPhase() const override { return Phase::Physics; }

        void OnStart() override;
        void OnUpdate(float dt) override;
        void OnStop() override;


        void AddImpulse(entt::entity entity, const glm::vec3& impulse);

        void AddForce(entt::entity entity, const glm::vec3& force);

        // PhysicsSystem.hpp
        RaycastHit Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, JPH::BodyID ignoreID = JPH::BodyID());
		bool isGrounded(entt::entity entity, float maxDistance = 0.1f);
    private:

        void OnRigidBodyCreated(entt::registry& reg, entt::entity entity);
        void OnRigidBodyDestroyed(entt::registry& reg, entt::entity entity);

    private:
        PhysicsManager _manager;
    };

} // namespace Kiki