#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace Kiki {

    struct Bone {
        std::string name;      // 骨头名字（用于匹配动画）
        int parentIndex = -1;  // 父骨头索引（-1 = root）

        glm::mat4 inverseBind = glm::mat4(1.0f);

        glm::mat4 localBindTransform = glm::mat4(1.0f);
    };

    struct Skeleton {
        std::vector<Bone> bones;

        // 查找 bone index（动画会用）
        int FindBoneIndex(const std::string& name) const {
            for (int i = 0; i < (int)bones.size(); i++) {
                if (bones[i].name == name) {
                    return i;
                }
            }
            return -1;
        }
    };

}