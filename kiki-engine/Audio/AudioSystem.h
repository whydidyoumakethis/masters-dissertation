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

            // 告诉 miniaudio 我们使用的是右手坐标系 (Right-Handed)
            // 这点非常重要！OpenGL/Vulkan 通常是右手系，如果不匹配，声音左右会反
            ma_engine* engine = AudioManager::get().getEngine();
            if (engine) {
                // 假设你的引擎使用 Y 向上，Z 向外的标准坐标系
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

            // ==========================================
            // 1. 处理耳朵 (Listener)
            // ==========================================
            auto listenerView = registry.view<AudioListenerComponent, TransformComponent>();

            // 🔥 使用 EnTT 最优雅的 each() 结构化绑定，彻底解决爆红和类型推断问题！
            for (auto [entity, listener, transform] : listenerView.each()) {

                if (listener.active) {
                    // 修复 1：直接读取 public 变量
                    glm::vec3 pos = transform.position;
                    ma_engine_listener_set_position(engine, 0, pos.x, pos.y, pos.z);

                    // 修复 2：四元数魔法！
                    // 在 3D 数学中，用旋转四元数乘以“默认前向向量”，就能得出当前的实际朝向。
                    // 绝大多数 OpenGL/Vulkan 引擎的标准右手系，默认前方是 -Z 轴 (0, 0, -1)
                    glm::vec3 forward = transform.rotation * glm::vec3(0.0f, 0.0f, -1.0f);
                    ma_engine_listener_set_direction(engine, 0, forward.x, forward.y, forward.z);
                }
            }

            // ==========================================
            // 2. 处理喇叭 (Sources) - 初始化与坐标同步
            // ==========================================
            auto sourceView = registry.view<AudioSourceComponent, TransformComponent>();

            for (auto [entity, source, transform] : sourceView.each()) {

                // --- A. 初始化逻辑保持不变 ---
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

                // --- B. 同步坐标 ---
                if (source.soundHandle) {
                    // 修复 3：直接读取 position
                    glm::vec3 pos = transform.position;
                    ma_sound_set_position(source.soundHandle, pos.x, pos.y, pos.z);
                }
            }

            // ==========================================
            // 3. 处理 2D OneShot 请求和缓存清理 (这部分不用动)
            // ==========================================
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

            // 极其重要：清理所有 ECS 实体上绑定的 3D 声音句柄
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