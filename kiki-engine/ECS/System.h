#pragma once
#include <vector>
#include <memory>
#include <algorithm>
#include "GameObject.h"
#include "RenderManager.hpp"
#include "WindowInfo.hpp"
class System {
public:
    virtual ~System() = default;

	// control the execution order of systems
    enum class Phase {
        PreUpdate,
        Update,
        PostUpdate,
        Render,
        Physics
    };

    virtual Phase GetPhase() const { return Phase::Update; }
    virtual void  OnUpdate(float dt) = 0;

	// reserved for systems that need to perform setup or cleanup tasks when they are added or removed from the engine
    virtual void OnStart() {}
    virtual void OnStop() {}
};

class SystemScheduler {
public:
    template<typename T, typename... Args>
    T* RegisterSystem(Args&&... args) {
        auto sys = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = sys.get();
        _systems.push_back(std::move(sys));
		// sort by phase
        std::stable_sort(_systems.begin(), _systems.end(),
            [](const auto& a, const auto& b) {
                return a->GetPhase() < b->GetPhase();
            });
        ptr->OnStart();
        return ptr;
    }

    void Update(float dt) {
        for (auto& sys : _systems)
            sys->OnUpdate(dt);
    }

    void Shutdown() {
        for (auto& sys : _systems)
            sys->OnStop();
        _systems.clear();
    }

private:
    std::vector<std::unique_ptr<System>> _systems;
};

class TransformSystem : public System {
public:
    Phase GetPhase() const override { return Phase::PreUpdate; }

    void OnUpdate(float dt) override {
        auto view = World::Get().Query<TransformComponent>();
        for (auto [entity, transform] : view.each()) {
            if (!transform.dirty) continue;
			// TODO: calculate world matrix
            transform.dirty = false;
        }
    }
};
class RenderSystem : public System {
public:
    Phase GetPhase() const override { return Phase::Render; }

    void OnUpdate(float dt) override {
        // TODO
        // auto view = World::Get().Query<TransformComponent, MeshComponent, MaterialComponent>();
        // for (auto [e, transform, mesh, material] : view.each()) {
        //     _renderer->Draw(mesh, material, transform.worldMatrix);
        // }

        // Temporary game loop
        /*if (!glfwWindowShouldClose(window)) {
            
        }*/
        renderManager.nextFrame();
    }
    void OnStart() override {
        // Temporary game loop
        Kiki::RenderManager& renderManager = Kiki::RenderManager::get();
        Kiki::WindowInfo info;
        info.fullscreen = false;
        info.monitor = 0;
        // info.width = 0;
        // info.height = 0;
        // info.decorations = false;
        info.resizeable = false;

        renderManager.initialise(info);

        // GLFWwindow* window = renderManager.getWindow();
        // create temporary glfw callback
        //glfwSetKeyCallback(window, [](GLFWwindow* aWindow, int aKey, int /*aScanCode*/, int aAction, int /*aModifierFlags*/) {
        //    if (aKey == GLFW_KEY_ESCAPE && aAction == GLFW_PRESS) {
        //        glfwSetWindowShouldClose(aWindow, GLFW_TRUE);
        //    }});  
	}
    void OnStop() override {
        renderManager.shutdown();
        window = nullptr;
	}
private:
	// Renderer* _renderer;
    Kiki::RenderManager& renderManager = Kiki::RenderManager::get();
    GLFWwindow* window = nullptr;
};

class PhysicsSystem : public System {
public:
    Phase GetPhase() const override { return Phase::Physics; }

    void OnUpdate(float dt) override {
		// TODO: physics update
    }
};