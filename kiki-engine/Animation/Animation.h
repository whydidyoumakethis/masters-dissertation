#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Kiki {

    struct Keyframe {
        float time = 0.0f;

        glm::vec3 translation = glm::vec3(0.0f);
        glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 scale = glm::vec3(1.0f);
    };

    struct BoneTrack {
        std::vector<Keyframe> keyframes;
    };

    struct Animation {
        float duration = 0.0f;

        // 和 skeleton.bones 一一对应
        std::vector<BoneTrack> tracks;
    };

}