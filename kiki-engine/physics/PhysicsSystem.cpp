#include "physics/PhysicsSystem.hpp"
#include "physics/PhysicsComponents.hpp"
#include "physics/PhysicsUtils.hpp"

#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Body/BodyFilter.h>

#include "ECS/World.h"
#include <spdlog/spdlog.h>

namespace Kiki {

    void PhysicsSystem::OnStart() {

        _manager.Initialize();

        auto& reg = World::Get().Registry();

        //Bind EnTT signal
        reg.on_construct<RigidBodyComponent>().connect<&PhysicsSystem::OnRigidBodyCreated>(this);
        reg.on_destroy<RigidBodyComponent>().connect<&PhysicsSystem::OnRigidBodyDestroyed>(this);

        spdlog::info("PhysicsSystem started and signals connected.");
    }

    void PhysicsSystem::OnUpdate(float dt) {
        auto& reg = World::Get().Registry();
        auto& bodyInterface = _manager.GetBodyInterface();
        auto view = reg.view<TransformComponent, RigidBodyComponent>();

        for (auto [entity, transform, rb] : view.each()) {
            if (transform.dirty) {
                JPH::RVec3 jPos = bodyInterface.GetPosition(rb.bodyID);
                if (glm::distance(transform.position, ToGLM(jPos)) > 0.01f) {
                    bodyInterface.SetPositionAndRotation(
                        rb.bodyID, ToJPHR(transform.position), ToJPH(transform.rotation), JPH::EActivation::Activate
                    );
                }
            }
        }

        // Pure physics simulation
        _manager.Update(dt);

        //write back the results to ECS
        for (auto [entity, transform, rb] : view.each()) {
            if (rb.motionType == JPH::EMotionType::Dynamic) {
                bool isActive = bodyInterface.IsActive(rb.bodyID);

                if (!isActive) {
                    static bool loggedSleep = false;
                    if (!loggedSleep) {
                        spdlog::warn("=== [PHYSICS DEACTIVATED] Entity {} is now SLEEPING. ===", (uint32_t)entity);
                        loggedSleep = true;
                    }
                }

                if (isActive) {
                    JPH::RVec3 pos; JPH::Quat rot;
                    bodyInterface.GetPositionAndRotation(rb.bodyID, pos, rot);
                    transform.position = ToGLM(pos);
                    transform.rotation = ToGLM(rot);
                    transform.dirty = true;
                }
            }
        }
    }

    void PhysicsSystem::OnStop() {
        _manager.Shutdown();
        spdlog::info("PhysicsSystem shut down.");
    }

    void PhysicsSystem::AddImpulse(entt::entity entity, const glm::vec3& impulse) {
        auto& reg = World::Get().Registry();
        if (auto* rb = reg.try_get<RigidBodyComponent>(entity)) {
            if (!rb->bodyID.IsInvalid()) {
                _manager.GetBodyInterface().AddImpulse(rb->bodyID, ToJPH(impulse));
                spdlog::info("impulse!!!");
            }
        }
    }

    void PhysicsSystem::AddForce(entt::entity entity, const glm::vec3& force) {
        auto& reg = World::Get().Registry();
        if (auto* rb = reg.try_get<RigidBodyComponent>(entity)) {
            if (!rb->bodyID.IsInvalid()) {
                _manager.GetBodyInterface().AddForce(rb->bodyID, ToJPH(force));
                spdlog::info("force!!!");
            }
        }
    }

    void PhysicsSystem::OnRigidBodyCreated(entt::registry& reg, entt::entity entity) {
        auto& rb = reg.get<RigidBodyComponent>(entity);
        auto& transform = reg.get<TransformComponent>(entity);

        JPH::ShapeRefC shape;
        
        if (auto* meshColl = reg.try_get<MeshColliderComponent>(entity)) {
            shape = meshColl->shape;
            spdlog::info("Entity {} using pre-computed MeshCollider.", (uint32_t)entity);
        }
        else if (auto* capsule = reg.try_get<CapsuleColliderComponent>(entity)) {
            shape = new JPH::CapsuleShape(capsule->radius, capsule->height);
		}
        else if (auto* box = reg.try_get<BoxColliderComponent>(entity)) {
            shape = new JPH::BoxShape(ToJPH(box->halfExtents));
        }
        else if (auto* sphere = reg.try_get<SphereColliderComponent>(entity)) {
            shape = new JPH::SphereShape(sphere->radius);
        }
        else {
            shape = new JPH::BoxShape(JPH::Vec3(0.5f, 0.5f, 0.5f));
            spdlog::warn("Entity {} has RigidBody but no Collider. Created default box shape.", (uint32_t)entity);
        }

        JPH::BodyCreationSettings settings(
            shape,
            ToJPHR(transform.position),
            ToJPH(transform.rotation),
            rb.motionType,
            rb.layer
        );

        settings.mFriction = rb.friction;
        settings.mRestitution = rb.restitution;
        settings.mIsSensor = rb.isSensor;

        auto& bi = _manager.GetBodyInterface();
        JPH::Body* body = bi.CreateBody(settings);
        bi.AddBody(body->GetID(), JPH::EActivation::Activate);

        rb.bodyID = body->GetID();
        body->SetUserData((JPH::uint64)entity);
    }

    void PhysicsSystem::OnRigidBodyDestroyed(entt::registry& reg, entt::entity entity) {
        auto& rb = reg.get<RigidBodyComponent>(entity);
        if (!rb.bodyID.IsInvalid()) {
            auto& bi = _manager.GetBodyInterface();
            bi.RemoveBody(rb.bodyID);
            bi.DestroyBody(rb.bodyID);
        }
    }

    RaycastHit PhysicsSystem::Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, JPH::BodyID ignoreID) {
        RaycastHit result;

        JPH::RRayCast ray;
        ray.mOrigin = ToJPHR(origin);
        ray.mDirection = ToJPH(direction * maxDistance);

        JPH::IgnoreSingleBodyFilter bodyFilter(ignoreID);

        JPH::RayCastResult joltResult;

        bool hit = _manager.GetSystem()->GetNarrowPhaseQuery().CastRay(
            ray,
            joltResult,
            {},
            {},
            bodyFilter
        );

        if (hit) {
            result.hasHit = true;
            result.distance = joltResult.mFraction * maxDistance;
            result.position = origin + direction * result.distance;

            auto& reg = World::Get().Registry();
            auto view = reg.view<RigidBodyComponent>();
            for (auto [ent, rb] : view.each()) {
                if (rb.bodyID == joltResult.mBodyID) {
                    result.entity = ent;

                    JPH::BodyLockRead lock(_manager.GetSystem()->GetBodyLockInterface(), joltResult.mBodyID);
                    if (lock.Succeeded()) {
                        result.normal = ToGLM(lock.GetBody().GetWorldSpaceSurfaceNormal(joltResult.mSubShapeID2, ray.GetPointOnRay(joltResult.mFraction)));
                    }
                    break;
                }
            }
        }

        return result;
    }

} // namespace Kiki