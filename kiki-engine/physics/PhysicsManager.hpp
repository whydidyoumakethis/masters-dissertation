#pragma once

#include <Jolt/Jolt.h>

#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include "physics/PhysicsContactListener.hpp"

#include <memory>

namespace Kiki {

	class PhysicsManager {
	public:
		PhysicsManager() = default;
		~PhysicsManager();

		void Initialize();

		void Update(float deltaTime);

		void Shutdown();

        JPH::BodyInterface& GetBodyInterface() { return m_PhysicsSystem->GetBodyInterface(); }

        JPH::PhysicsSystem* GetSystem() { return m_PhysicsSystem.get(); }

    private:
        std::unique_ptr<JPH::PhysicsSystem> m_PhysicsSystem;

        //TempAllocator & JobSystem
        std::unique_ptr<JPH::TempAllocatorImpl>     m_TempAllocator;
        std::unique_ptr<JPH::JobSystemThreadPool>   m_JobSystem;
        // PhysicsManager.hpp
        std::unique_ptr<PhysicsContactListener> m_ContactListener;

        class BPLayerInterfaceImpl* m_BPLayerInterface = nullptr;
        class ObjectVsBroadPhaseLayerFilterImpl* m_OBPFilter = nullptr;
        class ObjectLayerPairFilterImpl* m_OOPFilter = nullptr;

        bool m_IsInitialized = false;
    };
}