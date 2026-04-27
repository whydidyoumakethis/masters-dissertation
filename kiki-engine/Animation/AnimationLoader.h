#pragma once

#include "Skeleton.h"
#include "Animation.h"

#include <assimp/scene.h>
#include <memory>

namespace Kiki {

    class AnimationLoader {
    public:
        static std::unique_ptr<Skeleton> LoadSkeleton(const aiScene* scene);

        static std::unique_ptr<Animation> LoadAnimation(
            const aiScene* scene,
            const Skeleton& skeleton,
            int animationIndex = 0
        );
    };

}