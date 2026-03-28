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
            if (rb.motionType == JPH::EMotionType::Dynamic) {
                bodyInterface.SetPositionAndRotation(
                    rb.bodyID, ToJPHR(transform.position), ToJPH(transform.rotation), JPH::EActivation::Activate
                );
            }
        }

        // Pure physics simulation
        _manager.Update(dt);

        //write back the results to ECS
        for (auto [entity, transform, rb] : view.each()) {
            if (rb.motionType == JPH::EMotionType::Dynamic) {
                JPH::RVec3 pos; JPH::Quat rot;
                bodyInterface.GetPositionAndRotation(rb.bodyID, pos, rot);

                transform.position = ToGLM(pos);
                transform.rotation = ToGLM(rot);
				transform.dirty = true; // Mark dirty to update world matrix in TransformSystem
                //glm::mat4 translation = glm::translate(glm::mat4(1.0f), transform.position);
                //glm::mat4 rotation = glm::mat4_cast(transform.rotation); 
                //glm::mat4 scale = glm::scale(glm::mat4(1.0f), transform.scale);

                //transform.worldMatrix = translation * rotation * scale;

                //spdlog::info("Entity ID: {} | Pos: X={:.2f}, Y={:.2f}, Z={:.2f}",
                //    (uint32_t)entity, transform.position.x, transform.position.y, transform.position.z);

                //---------------------Hard - coded ray cases----------------------
                // Simulation: A 20-meter-long ray is fired vertically downwards from the position of the sphere.
                //auto hit = this->Raycast(transform.position, glm::vec3(0, -1, 0), 20.0f, rb.bodyID);
                //if (hit.hasHit) {
                    // Real-time height of the printed ball above the ground
                    //spdlog::info("Ball is {:.2f} meters above Entity {}", hit.distance, (uint32_t)hit.entity);
                //}
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