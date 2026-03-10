#pragma once
#include "ECS/World.h"
#include "ECS/GameObject.h"
#include "ECS/System.h"

namespace Kiki {
	class Engine {
	public:
		void Init() {
			_scheduler.RegisterSystem<TransformSystem>();
			_scheduler.RegisterSystem<RenderSystem>();
			_scheduler.RegisterSystem<PhysicsSystem>();
		}

		// for game layer to register systems
		template<typename T, typename... Args>
		T* RegisterSystem(Args&&... args) {
			return _scheduler.RegisterSystem<T>(std::forward<Args>(args)...);
		}

		void Run() {
			_running = true;
			while (_running && !glfwWindowShouldClose(RenderManager::get().getWindow())) {
				// float dt = _timer.Tick();
				float dt = 0.016f; // TODO: calculate delta time
				// TODO: MessageCenter::Flush();
				_scheduler.Update(dt);
				World::Get().FlushDestroy();
			}

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