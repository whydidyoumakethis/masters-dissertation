#pragma once
#include "ECS/System.h"
#include "AudioManager.h"
#include <spdlog/spdlog.h>
#include <string>
#include <vector>
//#include <mutex>
#include <filesystem>
#include "AudioCache.h"
#include <list>
#include "AudioComponents.h"

namespace Kiki {

    struct AudioRequest {
        std::string clipPath;
        bool is2D;
    };

    class AudioSystem : public System {
    public:
        Phase GetPhase() const override { return Phase::PostUpdate; }

        void OnStart() override {
            AudioManager::get().initialise();

            // tell miniaudio we use Right-Handed
            ma_engine* engine = AudioManager::get().getEngine();
            if (engine) {
                ma_engine_listener_set_world_up(engine, 0, 0, 1, 0); // Y轴朝上
            }

            spdlog::info("AudioSystem: Started with in-memory caching.");
        }

        static void PlayOneShot(const std::string& path, bool is2D = true) {
            std::lock_guard<std::mutex> lock(_queueMutex);
            _requestQueue.push_back({ path, is2D });
        }

        void OnUpdate(float dt) override {
            ma_engine* engine = AudioManager::get().getEngine();
            if (!engine) return;

            auto& registry = World::Get().Registry();

            // listener
            auto listenerView = registry.view<AudioListenerComponent, TransformComponent>();

            for (auto [entity, listener, transform] : listenerView.each()) {

                if (listener.active) {
                    glm::vec3 pos = transform.position;
                    ma_engine_listener_set_position(engine, 0, pos.x, pos.y, pos.z);

                    glm::vec3 forward = transform.rotation * glm::vec3(0.0f, 0.0f, -1.0f);
                    ma_engine_listener_set_direction(engine, 0, forward.x, forward.y, forward.z);
                }
            }

            // Sources
            auto sourceView = registry.view<AudioSourceComponent, TransformComponent>();

            for (auto [entity, source, transform] : sourceView.each()) {

                if (source.soundHandle == nullptr) {
                    AudioClip* clip = AudioCache::get().getOrLoad(source.clipPath);
                    if (clip && clip->isValid) {
                        source.soundHandle = new ma_sound();
                        ma_sound_init_copy(engine, &clip->masterSound, 0, NULL, source.soundHandle);

                        ma_sound_set_min_distance(source.soundHandle, source.minDistance);
                        ma_sound_set_max_distance(source.soundHandle, source.maxDistance);
                        ma_sound_set_looping(source.soundHandle, source.isLooping ? MA_TRUE : MA_FALSE);
                        ma_sound_set_volume(source.soundHandle, source.volume);
                        ma_sound_set_pitch(source.soundHandle, source.pitch);

                        ma_sound_set_spatialization_enabled(source.soundHandle, MA_TRUE);

                        if (source.playOnAwake) {
                            ma_sound_start(source.soundHandle);
                        }
                    }
                }

                if (source.soundHandle) {
                    glm::vec3 pos = transform.position;
                    ma_sound_set_position(source.soundHandle, pos.x, pos.y, pos.z);
                }
            }

            // 2D OneShot
            {
                std::lock_guard<std::mutex> lock(_queueMutex);
                for (const auto& req : _requestQueue) {
                    AudioClip* clip = AudioCache::get().getOrLoad(req.clipPath);
                    if (clip && clip->isValid) {
                        ma_sound* newSound = new ma_sound();
                        ma_sound_init_copy(engine, &clip->masterSound, 0, NULL, newSound);
                        ma_sound_set_spatialization_enabled(newSound, MA_FALSE);
                        ma_sound_start(newSound);
                        _activeSounds.push_back(newSound);
                    }
                }
                _requestQueue.clear();
            }

            for (auto it = _activeSounds.begin(); it != _activeSounds.end(); ) {
                ma_sound* sound = *it;
                if (ma_sound_at_end(sound)) {
                    ma_sound_uninit(sound);
                    delete sound;
                    it = _activeSounds.erase(it);
                }
                else {
                    ++it;
                }
            }
        }

        void OnStop() override {
            spdlog::info("AudioSystem: Shutting down logic and clearing cache...");

            for (ma_sound* sound : _activeSounds) {
                ma_sound_uninit(sound);
                delete sound;
            }
            _activeSounds.clear();

            {
                std::lock_guard<std::mutex> lock(_queueMutex);
                _requestQueue.clear();
            }

            auto sourceView = World::Get().Registry().view<AudioSourceComponent>();
            for (auto entity : sourceView) {
                auto& source = sourceView.get<AudioSourceComponent>(entity);
                if (source.soundHandle) {
                    ma_sound_uninit(source.soundHandle);
                    delete source.soundHandle;
                    source.soundHandle = nullptr;
                }
            }

            AudioCache::get().clear();
        }

    private:
        inline static std::vector<AudioRequest> _requestQueue;
        inline static std::mutex _queueMutex;

        std::list<ma_sound*> _activeSounds;
    };
}