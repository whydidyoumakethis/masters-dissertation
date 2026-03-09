#include <PhysicsManager.hpp>
#include <iostream>
#include "physics/PhysicsContactListener.hpp"

JPH_SUPPRESS_WARNINGS

// All Jolt symbols are in the JPH namespace
using namespace JPH;

using namespace JPH::literals;

namespace Kiki {

    namespace Layers
    {
        static constexpr ObjectLayer NON_MOVING = 0;
        static constexpr ObjectLayer MOVING = 1;
        static constexpr ObjectLayer NUM_LAYERS = 2;
    };

    // Determine whether collisions are allowed between two object layers
    class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter {
    public:
        virtual bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override {
            switch(inObject1)//Determine based on the first ObjectLayer
            {
		    case Layers::NON_MOVING:
                return inObject2 == Layers::MOVING; // Non moving only collides with moving
            case Layers::MOVING:
                return true; // Moving collides with everything
            default:
                JPH_ASSERT(false);
                return false;
            }
        }
    };

    // Coarse testing phase layer definition
    namespace BroadPhaseLayers {
        static constexpr BroadPhaseLayer NON_MOVING(0);
        static constexpr BroadPhaseLayer MOVING(1);
        static constexpr uint NUM_LAYERS(2);
    };

    // Mapping object layer to coarse measurement layer
    class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface {
    public:
        BPLayerInterfaceImpl() {
            // Create a mapping table from the object layer to the coarse measurement layer.
            mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
            mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
        }

        virtual uint GetNumBroadPhaseLayers() const override {
            return BroadPhaseLayers::NUM_LAYERS;
        }

        virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override {
            JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
            return mObjectToBroadPhase[inLayer];
        }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override {
            switch ((BroadPhaseLayer::Type)inLayer) {
            case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
            case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		return "MOVING";
            default: JPH_ASSERT(false); return "INVALID";
            }
        }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED
        // ---------------------------------------

    private:
        BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
    };

    // Determines whether the object layer collides with the coarse test layer.
    class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter {
    public:
        virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override {
            switch (inLayer1) {
            case Layers::NON_MOVING: return inLayer2 == BroadPhaseLayers::MOVING;
            case Layers::MOVING:     return true;
            default:                 return false;
            }
        }
    };

    //Destructor
    PhysicsManager::~PhysicsManager() {

        Shutdown();
    }

    void PhysicsManager::Initialize() {
        if (m_IsInitialized) return;

        // TODO: 1.(RegisterDefaultAllocator)
        RegisterDefaultAllocator();

        // TODO: 2.(Factory::sInstance)
        Factory::sInstance = new Factory();

        // TODO: 3. (RegisterTypes)
        RegisterTypes();

        // TODO: 4. TempAllocator & JobSystem
        m_TempAllocator = std::make_unique<TempAllocatorImpl>(10 * 1024 * 1024);
        m_JobSystem = std::make_unique<JobSystemThreadPool>(
            cMaxPhysicsJobs,
            cMaxPhysicsBarriers,
            std::thread::hardware_concurrency() - 1
        );

        // TODO: 5. Instantiate the filter layer (m_BPLayerInterface...)
        m_BPLayerInterface = new BPLayerInterfaceImpl();
        m_OBPFilter = new ObjectVsBroadPhaseLayerFilterImpl();
        m_OOPFilter = new ObjectLayerPairFilterImpl();

        // TODO: 6. initialize PhysicsSystem (m_PhysicsSystem->Init)
        const uint cMaxBodies = 1024;
        const uint cNumBodyMutexes = 0; 
        const uint cMaxBodyPairs = 1024;
        const uint cMaxContactConstraints = 1024;

        m_PhysicsSystem = std::make_unique<PhysicsSystem>();
        m_PhysicsSystem->Init(
            cMaxBodies,
            cNumBodyMutexes,
            cMaxBodyPairs,
            cMaxContactConstraints,
            *m_BPLayerInterface,
            *m_OBPFilter,
            *m_OOPFilter
        );

        // 7. Instantiate the listener and register it in the system.
        m_ContactListener = std::make_unique<PhysicsContactListener>();
        m_PhysicsSystem->SetContactListener(m_ContactListener.get());

        std::cout << "KikiEngine: Jolt Physics System Initialized." << std::endl;

        m_IsInitialized = true;
    }

    void PhysicsManager::Update(float deltaTime) {
        if (!m_IsInitialized) return;

        const int cCollisionSteps = 1;
        m_PhysicsSystem->Update(deltaTime, cCollisionSteps, m_TempAllocator.get(), m_JobSystem.get());
        // std::cout << "KikiEngine:Physics Update." << std::endl;
    }

    void PhysicsManager::Shutdown() {
        if (!m_IsInitialized) return;
        //Some cleaning work...
        delete m_BPLayerInterface; m_BPLayerInterface = nullptr;
        delete m_OBPFilter;        m_OBPFilter = nullptr;
        delete m_OOPFilter;        m_OOPFilter = nullptr;

        m_PhysicsSystem.reset();
        m_JobSystem.reset();
        m_TempAllocator.reset();

        UnregisterTypes();
        delete Factory::sInstance;
        Factory::sInstance = nullptr;

        m_IsInitialized = false;

        m_IsInitialized = false;
    }
}