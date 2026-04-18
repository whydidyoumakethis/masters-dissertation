#pragma once

#include "Skeleton.h"
#include "Animation.h"

#include <vector>
#include <glm/glm.hpp>

namespace Kiki {

    class Animator {
    public:
        float currentTime = 0.0f;
        bool looping = true;

        std::vector<glm::mat4> localMatrices;
        std::vector<glm::mat4> globalMatrices;
        std::vector<glm::mat4> finalMatrices;

        void Update(float dt, const Skeleton& skeleton, const Animation& animation);
    };

}