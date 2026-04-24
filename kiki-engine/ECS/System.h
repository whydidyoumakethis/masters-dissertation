#pragma once
#include <vector>
#include <memory>
#include <algorithm>
#include "GameObject.h"
#include "RenderManager.hpp"
#include "WindowInfo.hpp"
#include "InputManager.hpp"
#include <spdlog/spdlog.h>
#include <iostream>

class System {
public:
    virtual ~System() = default;

	// control the execution order of systems
    enum class Phase {
        
        PreUpdate,
        Update,
        Physics,
        PostUpdate,
        Render,
        Input
    };

    virtual Phase GetPhase() const { return Phase::Update; }
    virtual void  OnUpdate(float dt) = 0;

	// reserved for systems that need to perform setup or cleanup tasks when they are added or removed from the engine
    virtual void OnStart() {}
    virtual void OnStop() {}
};

class SystemScheduler {
public:

	// where OnStart is called immediately
	// order of execution is determined by the phase returned by GetPhase() and the order of registration (stable sort)
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

    // do not use GetSystem, it increases coupling between systems and breaks the ECS architecture
	// if you need a tool function, try to package it as a service. Check PhysicsService in PhysicsSystem.hpp for an example
    template<typename T>
    T* GetSystem() {
        for (auto& system : _systems) {
            T* target = dynamic_cast<T*>(system.get());
            if (target) return target;
        }
        return nullptr;
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
    void printSystemOrder() {
        spdlog::info("Current system execution order:");
        for (const auto& sys : _systems) {
            spdlog::info("- {}", typeid(*sys).name());
        }
	}

private:
    std::vector<std::unique_ptr<System>> _systems;
};

class TransformSystem : public System {
public:
    Phase GetPhase() const override { return Phase::PostUpdate; }

    void OnUpdate(float dt) override {
        auto view = World::Get().Query<TransformComponent>();
        for (auto [entity, transform] : view.each()) {
            if (!transform.dirty) continue;
			// calculate world matrix without parent-child relationship for now
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), transform.position);
            glm::mat4 rotation = glm::mat4_cast(transform.rotation);
            glm::mat4 scale = glm::scale(glm::mat4(1.0f), transform.scale);

            transform.worldMatrix = translation * rotation * scale;
            transform.dirty = false;
			//spdlog::info("location of entity {}: x={}, y={}, z={}", (uint32_t)entity, transform.position.x, transform.position.y, transform.position.z);
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
        // info.fullscreen = false;
        // info.monitor = 0;
        info.width = 0;
        info.height = 0;
        // info.decorations = false;
        // info.resizeable = false;

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


