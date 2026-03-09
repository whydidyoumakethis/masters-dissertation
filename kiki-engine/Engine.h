#pragma once
#include "ECS/World.h"
#include "ECS/GameObject.h"
#include "ECS/System.h"
#include "input/InputSystem.hpp"
#include "PhysicsSystem.cpp"

#include <spdlog/spdlog.h>

namespace Kiki {
	class Engine {
	public:
		void Init() {
			_scheduler.RegisterSystem<TransformSystem>();
			_scheduler.RegisterSystem<RenderSystem>();
			_scheduler.RegisterSystem<Kiki::InputSystem>();
			_scheduler.RegisterSystem<Kiki::PhysicsSystem>();
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
			while (_running) {
				// float dt = _timer.Tick();
				float dt = 0.016f; // TODO: calculate delta time
				// TODO: MessageCenter::Flush();
				_scheduler.Update(dt);
				World::Get().FlushDestroy();
			}
		}
	void Quit() {
		_running = false;
	}
	private:
		SystemScheduler _scheduler;
		bool            _running = false;
	};
}