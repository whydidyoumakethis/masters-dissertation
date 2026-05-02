#pragma once
#include <string>
#include <miniaudio.h> 

namespace Kiki {


    struct AudioListenerComponent {
        bool active = true; // 预留一个开关，方便你在多个摄像机之间切换监听权
    };


    // 挂载在任何需要发声的 3D 实体上（比如火把、NPC、瀑布）
    struct AudioSourceComponent {
        std::string clipPath;        // 资源路径，例如 "sounds/fire.mp3"

        // --- 基础控制属性 ---
        float volume = 1.0f;         // 音量 (0.0 到 1.0+)
        float pitch = 1.0f;          // 音调 (1.0 是原声，大于 1 变尖锐，小于 1 变低沉)
        bool isLooping = false;      // 是否循环播放 (火把、水流等环境音通常为 true)
        bool playOnAwake = true;     // 是否在组件挂载的第一帧自动开始播放

        // --- 3D 核心衰减属性 ---
        // miniaudio 默认使用“倒数衰减模型 (Inverse Tapering)”
        float minDistance = 1.0f;    // 最小距离：在这个距离以内，你听到的声音都是 100% 音量。
        float maxDistance = 20.0f;   // 最大距离：超出这个距离，声音衰减为 0，彻底听不见。

        // --- 运行时句柄 (系统专用) ---
        ma_sound* soundHandle = nullptr;
    };

}