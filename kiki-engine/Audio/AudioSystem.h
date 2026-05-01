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
            spdlog::info("AudioSystem: Started with in-memory caching.");
        }

        static void PlayOneShot(const std::string& path, bool is2D = true) {
            std::lock_guard<std::mutex> lock(_queueMutex);
            _requestQueue.push_back({ path, is2D });
        }

        void OnUpdate(float dt) override {
            ma_engine* engine = AudioManager::get().getEngine();
            if (!engine) return;

            {
                std::lock_guard<std::mutex> lock(_queueMutex);
                for (const auto& req : _requestQueue) {

                    // Request sound data from memory using the cache.
                    AudioClip* clip = AudioCache::get().getOrLoad(req.clipPath);
                    if (clip && clip->isValid) {

                        // Dynamically create a new sound instance.
                        ma_sound* newSound = new ma_sound();

                        // A copy is "cloned" from the parent database in memory.
                        ma_sound_init_copy(engine, &clip->masterSound, 0, NULL, newSound);

                        // play
                        ma_sound_start(newSound);

                        // put it in list
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

            AudioCache::get().clear();
        }

    private:
        inline static std::vector<AudioRequest> _requestQueue;
        inline static std::mutex _queueMutex;

        std::list<ma_sound*> _activeSounds;
    };
}