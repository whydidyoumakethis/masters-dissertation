#pragma once
#include "ECS/World.h"
#include "ECS/GameObject.h"
#include "ECS/System.h"
#include "input/InputSystem.hpp"
#include "PhysicsSystem.cpp"
#include "renderer/SceneManager.hpp"

#include "../debugging/DebugCamera.hpp"

#include "GltfLoader/GltfLoaderAssimp.h"
#include <spdlog/spdlog.h>

#include <chrono>

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

			// Temp addition for debug cam
			DebugCamera cam;
			RenderManager::get().setCamera(cam);

			auto& registry = World::Get().Registry();
			auto road = World::Get().CreateEntity();

			std::vector<float> p = {
                -1.f, 0.f, -6.f, // v0
                -1.f, 0.f, +6.f, // v1
                +1.f, 0.f, +6.f, // v2
                +1.f, 0.f, -6.f // v3
            };

            std::vector<std::uint32_t> i = { 0, 1, 2, 0, 2, 3 };

            std::vector<float> c = {
                0.f, -6.f, // t0
                0.f, +6.f, // t1
                1.f, +6.f, // t2
                1.f, -6.f // t3
            };
			//
			stbi_set_flip_vertically_on_load( 1 );

			// Load base image
			int baseWidthi, baseHeighti, baseChannelsi;

			stbi_uc* data = stbi_load( (std::filesystem::path(PROJECT_ROOT_PATH) / "games/demo/assets/asphalt.png").c_str(), &baseWidthi, &baseHeighti, &baseChannelsi, 4 /* want 4 c h a n n e l s = RGBA */);
			//
			registry.emplace<TransformComponent>(road);
			registry.emplace<MeshComponent>(road, SceneManager::get().createMesh(p, i, c));
			registry.emplace<MaterialComponent>(road, SceneManager::get().createMaterial(data, baseWidthi, baseHeighti, BlendMode::OPAQUE));

			auto test_cube = World::Get().CreateEntity();
			registry.emplace<TransformComponent>(test_cube);

			Mmesh mesh = Kiki::GltfLoaderAssimp::loadMesh(std::filesystem::path(PROJECT_ASSETS_PATH) / "test_cube_tex.glb");
			Mtexture texture = Kiki::GltfLoaderAssimp::loadTexture(std::filesystem::path(PROJECT_ASSETS_PATH) / "test_cube_tex.glb");

			registry.emplace<MeshComponent>(test_cube, SceneManager::get().createMesh(mesh.vertices, mesh.indices, mesh.uvs));
			const bool isCompressed = !texture.rawData.empty();
			unsigned char* texPtr = isCompressed
				? (unsigned char*)texture.rawData.data()
				: (unsigned char*)texture.data.data();
			int texSize = isCompressed
				? static_cast<int>(texture.rawData.size())
				: static_cast<int>(texture.data.size() * sizeof(RGBA));

			registry.emplace<MaterialComponent>(test_cube,
				SceneManager::get().createMaterial(texture.rawDataPtr, texture.width, texture.height, BlendMode::OPAQUE));

			Kiki::GltfLoaderAssimp::debugPrintMesh(mesh);
			Kiki::GltfLoaderAssimp::debugPrintTexture(texture);

			auto previousClock = std::chrono::steady_clock::now();

			while (_running && !glfwWindowShouldClose(RenderManager::get().getWindow())) {
				// float dt = _timer.Tick();
				auto const now = std::chrono::steady_clock::now();
				auto const dt = std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1>>>(now-previousClock).count(); // TODO: calculate delta time
				previousClock = now;
				// TODO: MessageCenter::Flush();
				cam.update(dt);
				glfwSetWindowShouldClose(RenderManager::get().getWindow(), InputManager::get().isKeyDown(GLFW_KEY_ESCAPE));
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