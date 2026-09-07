#pragma once
#include "ECS/World.h"
#include "ECS/GameObject.h"
#include "ECS/System.h"
#include "input/InputSystem.hpp"
#include "PhysicsSystem.cpp"
#include "renderer/SceneManager.hpp"
#include "Components/TransparencyComponent.hpp"
#include "Animation/AnimationSystem.h"
#include "Animation/SimpleAnimationSystem.h"

#include "../debugging/DebugCamera.hpp"
#include "debugging/DebugInterface.hpp"
#include "MessageCenter.h"
#include "GltfLoader/GltfLoaderAssimp.h"
#include <spdlog/spdlog.h>
#include "debugging/DebugSystem.hpp"
#include "interface/InterfaceSystem.hpp"
#include "Audio/AudioSystem.h"
#include "Audio/BGMController.h"

#include "Timer/Timer.h"


#include <chrono>
#include <thread>

namespace Kiki {
	class Engine {
	public:
		void Init() {
			_scheduler.RegisterSystem<Kiki::PhysicsSystem>();
			_scheduler.RegisterSystem<TransformSystem>();
			_scheduler.RegisterSystem<AnimationSystem>();
			_scheduler.RegisterSystem<RenderSystem>();
			_scheduler.RegisterSystem<Kiki::InputSystem>();
			_scheduler.RegisterSystem<Kiki::DebugSystem>();
			_scheduler.RegisterSystem<Kiki::InterfaceSystem>();
			_scheduler.RegisterSystem<SimpleAnimationSystem>();

			_scheduler.RegisterSystem<Kiki::AudioSystem>();
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
			_scheduler.printSystemOrder();
			auto& timer = Timer::get();
			auto& input = Kiki::InputManager::get();
			//timer.UseFixedTime(1.f / 200.f);
			bool paused = false;
			bool captureOverridesTimer = false;
			Timer::Mode capturePreviousTimerMode = timer.GetMode();
			float capturePreviousFixedDelta = timer.GetFixedDelta();

			auto restoreTimerAfterCapture = [&]() {
				if (!captureOverridesTimer) return;
				if (capturePreviousTimerMode == Timer::Mode::Fixed) {
					timer.UseFixedTime(capturePreviousFixedDelta);
				}
				else {
					timer.UseRealTime();
				}
				captureOverridesTimer = false;
			};

			auto startDeterministicCapture = [&]() {
				capturePreviousTimerMode = timer.GetMode();
				capturePreviousFixedDelta = timer.GetFixedDelta();
				timer.UseFixedTime(1.0f / 60.0f);
				captureOverridesTimer = true;

				if (RenderManager::get().togglePngFrameCapture(100, 50)) {
					spdlog::info(
						"[PNG Capture] Deterministic frame 0; frame 100 will be the first saved image"
					);
				}
				else {
					restoreTimerAfterCapture();
				}
			};

			 //Begin the deterministic timeline before the first simulation update. Every
			// launch therefore renders the same animation pose at capture frame 100.
			//startDeterministicCapture();

			while (_running && !glfwWindowShouldClose(RenderManager::get().getWindow())) {
				_scheduler.UpdatePhase(System::Phase::Input, 0.0f);
				if (input.isKeyJustDown(GLFW_KEY_F8)) {
					if (RenderManager::get().isPngFrameCaptureActive()) {
						spdlog::warn("[PNG Capture] Finish or cancel the F10 capture before pausing");
					}
					else {
						if (!paused) {
							paused = true;
							timer.Pause();
							spdlog::info("Engine paused");
						}
						else {
							paused = false;
							timer.Resume();
							spdlog::info("Engine resumed");
						}
					}
				}



				if (paused) {
					// Keep polling input so F8 can resume, but do not begin a frame.
					std::this_thread::sleep_for(std::chrono::milliseconds(8));
					continue;
				}
				const float dt = timer.Tick();

				MessageCenter::Flush();

				_scheduler.UpdateSimulation(dt);

				// Rendering occurs exactly once after simulation.
				 _scheduler.UpdatePhase(System::Phase::Render, dt);

				if (captureOverridesTimer && !RenderManager::get().isPngFrameCaptureActive()) {
					restoreTimerAfterCapture();
				}

				World::Get().FlushDestroy();

				//MessageCenter::Flush();
				//glfwSetWindowShouldClose(RenderManager::get().getWindow(), InputManager::get().isKeyJustDown(GLFW_KEY_ESCAPE) && InputManager::get().isCursorDisabledFunc());
				//_scheduler.Update(dt);
				//World::Get().FlushDestroy();
			}

			BGMController::get().Shutdown();
			AudioManager::get().shutdown();
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
