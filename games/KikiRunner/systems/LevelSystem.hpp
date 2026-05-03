#ifndef KIKIRUNNER_LEVELSYSTEM
#define KIKIRUNNER_LEVELSYSTEM

#include <kiki.h>

#include "events/RequestLevelChangeEvent.hpp"
#include "events/LevelLoadedEvent.hpp"

class LevelSystem : public System {
	public:
	Phase GetPhase() const override { return Phase::PreUpdate; }

	void OnStart() override {
		MessageCenter::Subscribe<RequestLevelChangeEvent, &LevelSystem::OnTriggerEnter>(this);
	}

	void OnUpdate(float dt) override {
		if (loaded) {
			loaded = false;
			MessageCenter::Publish(LevelLoadedEvent());
		}
	}

	void OnTriggerEnter(const RequestLevelChangeEvent& e) {
		std::thread([this, e]() {
			sceneManager.clearLevel();
			sceneManager.loadScene(Kiki::GltfLoaderAssimp::loadScene(e.levelPath));
			loaded = true;
		}).detach();
	}

	private:
	SceneManager& sceneManager = Kiki::SceneManager::get();
	bool loaded = false;
};

#endif