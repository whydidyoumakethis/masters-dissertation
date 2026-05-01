#pragma once
#include "ECS/System.h"
#include "AudioManager.h"
#include <spdlog/spdlog.h>
#include <string>
#include <vector>
#include <mutex>
#include <filesystem>

namespace Kiki {

    // 音频播放请求的数据结构
    struct AudioRequest {
        std::string clipPath;
        bool is2D;
    };

    class AudioSystem : public System {
    public:
        Phase GetPhase() const override { return Phase::PostUpdate; }

        void OnStart() override {
            AudioManager::get().initialise();
            spdlog::info("AudioSystem: Started and ready to receive PlayOneShot commands.");
        }

        static void PlayOneShot(const std::string& path, bool is2D = true) {
            std::lock_guard<std::mutex> lock(_queueMutex); // Multithreading
            _requestQueue.push_back({ path, is2D });
        }

        void OnUpdate(float dt) override {
            // Lock the queue and extract all the sounds that need to be played in this frame.
            std::lock_guard<std::mutex> lock(_queueMutex);

			if (_requestQueue.empty()) return; // no need to play, just return

            ma_engine* engine = AudioManager::get().getEngine();
            if (!engine) {
                _requestQueue.clear();
                return;
            }
            // play all the requested sounds
            for (const auto& req : _requestQueue) {
                std::filesystem::path audioPath = std::filesystem::path(PROJECT_ASSETS_PATH) / req.clipPath;
                std::string finalPath = audioPath.string();

                if (std::filesystem::exists(audioPath)) {
                    ma_engine_play_sound(engine, finalPath.c_str(), NULL);
                    spdlog::info("AudioSystem: Playing OneShot -> {}", req.clipPath);
                }
                else {
                    spdlog::warn("AudioSystem: File not found -> {}", finalPath);
                }
            }

			// clear the queue after processing
            _requestQueue.clear();
        }

        void OnStop() override {
            spdlog::info("AudioSystem: Shutting down logic...");
            std::lock_guard<std::mutex> lock(_queueMutex);
            _requestQueue.clear();
        }

    private:
        // avoid repeatly include them
        inline static std::vector<AudioRequest> _requestQueue;
        inline static std::mutex _queueMutex;
    };
}