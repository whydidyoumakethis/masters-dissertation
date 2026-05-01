#pragma once
#include <miniaudio.h>
#include <spdlog/spdlog.h>

namespace Kiki {
    class AudioManager {
    public:
        static AudioManager& get() {
            static AudioManager instance;
            return instance;
        }

        void initialise() {
            if (_isInitialized) return;

            //spdlog::info("AudioManager: Initializing hardware...");
            if (ma_engine_init(NULL, &_engine) != MA_SUCCESS) {
                spdlog::error("AudioManager: Failed to initialize audio engine.");
                return;
            }
            _isInitialized = true;
        }

        void shutdown() {
            if (!_isInitialized) return;

            //spdlog::info("AudioManager: Shutting down hardware safely...");
            ma_engine_uninit(&_engine);
            _isInitialized = false;
        }

        ma_engine* getEngine() {
            return &_engine;
        }

    private:
        AudioManager() = default;
        ~AudioManager() {
            shutdown();
        }

        ma_engine _engine;
        bool _isInitialized = false;
    };
}