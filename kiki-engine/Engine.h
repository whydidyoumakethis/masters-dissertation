#pragma once
#include "ECS/World.h"
#include "ECS/GameObject.h"
#include "ECS/System.h"
#include "input/InputSystem.hpp"
#include "PhysicsSystem.cpp"
#include "renderer/SceneManager.hpp"
#include "Components/TransparencyComponent.hpp"

#include "../debugging/DebugCamera.hpp"
#include "debugging/DebugInterface.hpp"
#include "MessageCenter.h"
#include "GltfLoader/GltfLoaderAssimp.h"
#include <spdlog/spdlog.h>

#include <chrono>

namespace Kiki {
	class Engine {
	public:
		void Init() {
			_scheduler.RegisterSystem<Kiki::PhysicsSystem>();
			_scheduler.RegisterSystem<TransformSystem>();
			_scheduler.RegisterSystem<RenderSystem>();
			_scheduler.RegisterSystem<Kiki::InputSystem>();
			
		}

		// for game layer to register systems
		template<typename T, typename... Args>
		T* RegisterSystem(Args&&... args) {
			return _scheduler.RegisterSystem<T>(std::forward<Args>(args)...);
		}

		template<typename T>
		T* GetSystem() {
			return _scheduler.GetSystem<T>();
		}

		void Run() {
			_running = true;

			// Temp addition for debug cam
			DebugCamera cam;
			RenderManager::get().setCamera(cam);

#			ifndef NDEBUG
			DebugInterface& debugInterface = Kiki::DebugInterface::get();
			debugInterface.initialise();
#			endif

			auto previousClock = std::chrono::steady_clock::now();

			while (_running && !glfwWindowShouldClose(RenderManager::get().getWindow())) {
				// float dt = _timer.Tick();
				auto const now = std::chrono::steady_clock::now();
				auto const dt = std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1>>>(now-previousClock).count(); // TODO: calculate delta time
				previousClock = now;
				MessageCenter::Flush();
				cam.update(dt);
#				ifndef NDEBUG
				debugInterface.update(dt);
#				endif
				//glfwSetWindowShouldClose(RenderManager::get().getWindow(), InputManager::get().isKeyJustDown(GLFW_KEY_ESCAPE) && InputManager::get().isCursorDisabledFunc());
				_scheduler.Update(dt);
				World::Get().FlushDestroy();
			}

			debugInterface.shutdown();
			RenderManager::get().shutdown(); // temp addition so i can check shutdown code
		}
	void Quit() {
		_running = false;
	}
	private:
		SystemScheduler _scheduler;
		bool            _running = false;
	};
}