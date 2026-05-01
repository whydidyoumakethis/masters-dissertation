#pragma once
#include <miniaudio.h>
#include <spdlog/spdlog.h>
#include <string>
#include <unordered_map>
#include <filesystem>
#include "AudioManager.h"

namespace Kiki {

    struct AudioClip {
        ma_sound masterSound; 
        bool isValid = false;

        ~AudioClip() {
            if (isValid) {
                ma_sound_uninit(&masterSound);
            }
        }
    };

    class AudioCache {
    public:
        static AudioCache& get() {
            static AudioCache instance;
            return instance;
        }

        AudioClip* getOrLoad(const std::string& relativePath) {
            if (_cache.find(relativePath) != _cache.end()) {
                return _cache[relativePath].get();
            }

            std::filesystem::path audioPath = std::filesystem::path(PROJECT_ASSETS_PATH) / relativePath;
            std::string finalPath = audioPath.string();

            if (!std::filesystem::exists(audioPath)) {
                spdlog::error("AudioCache: File not found -> {}", finalPath);
                return nullptr;
            }

            ma_engine* engine = AudioManager::get().getEngine();
            auto clip = std::make_unique<AudioClip>();

            ma_result result = ma_sound_init_from_file(
                engine,
                finalPath.c_str(),
                MA_SOUND_FLAG_DECODE, //core
                NULL,
                NULL,
                &clip->masterSound
            );

            if (result != MA_SUCCESS) {
                spdlog::error("AudioCache: Failed to decode audio file -> {}", finalPath);
                return nullptr;
            }

            clip->isValid = true;
            spdlog::info("AudioCache: Successfully loaded and decoded into RAM -> {}", relativePath);

            AudioClip* ptr = clip.get();
            _cache[relativePath] = std::move(clip);
            return ptr;
        }

        void clear() {
            _cache.clear();
            spdlog::info("AudioCache: Cleared all audio memory.");
        }

    private:
        AudioCache() = default;
        std::unordered_map<std::string, std::unique_ptr<AudioClip>> _cache;
    };
}