#pragma once
#include <miniaudio.h>
#include <spdlog/spdlog.h>
#include <string>
#include <filesystem>
#include "AudioManager.h"

namespace Kiki {

    class BGMController {
    public:
        static BGMController& get() {
            static BGMController instance;
            return instance;
        }

		// play a new BGM, stop last one if exists.
        void Play(const std::string& relativePath, float volume = 1.0f, bool loop = true) {
            Stop();

            ma_engine* engine = AudioManager::get().getEngine();
            if (!engine) return;

            std::filesystem::path audioPath = std::filesystem::path(PROJECT_ASSETS_PATH) / relativePath;
            std::string finalPath = audioPath.string();

            if (!std::filesystem::exists(audioPath)) {
                spdlog::error("BGMController: BGM file not found -> {}", finalPath);
                return;
            }

            ma_result result = ma_sound_init_from_file(
                engine,
                finalPath.c_str(),
                MA_SOUND_FLAG_STREAM,
                NULL,
                NULL,
                &_bgmSound
            );

            if (result == MA_SUCCESS) {
                _hasValidSound = true;
                ma_sound_set_volume(&_bgmSound, volume);
                ma_sound_set_looping(&_bgmSound, loop ? MA_TRUE : MA_FALSE);
                ma_sound_start(&_bgmSound);
                spdlog::info("BGMController: Now playing BGM -> {}", relativePath);
            }
            else {
                spdlog::error("BGMController: Failed to load BGM -> {}", relativePath);
            }
        }

        // stop play
        void Stop() {
            if (_hasValidSound) {
                ma_sound_uninit(&_bgmSound);
                _hasValidSound = false;
                spdlog::info("BGMController: BGM stopped.");
            }
        }

        // pause
        void Pause() {
            if (_hasValidSound && ma_sound_is_playing(&_bgmSound)) {
                ma_sound_stop(&_bgmSound); 
                spdlog::info("BGMController: BGM paused.");
            }
        }

        // resume
        void Resume() {
            if (_hasValidSound && !ma_sound_is_playing(&_bgmSound)) {
                ma_sound_start(&_bgmSound);
                spdlog::info("BGMController: BGM resumed.");
            }
        }

		// set volume, 0.0f is silent, 1.0f is original volume, >1.0f is amplified
        void SetVolume(float volume) {
            if (_hasValidSound) {
                ma_sound_set_volume(&_bgmSound, volume);
            }
        }

        void Shutdown() {
            Stop();
        }

    private:
        BGMController() = default;
        ~BGMController() { Shutdown(); }

        ma_sound _bgmSound;
        bool _hasValidSound = false;
    };
}