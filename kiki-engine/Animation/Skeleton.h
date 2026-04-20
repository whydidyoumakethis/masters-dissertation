#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace Kiki {

    struct Bone {
        std::string name;   
        int parentIndex = -1; 

        glm::mat4 inverseBind = glm::mat4(1.0f);

        glm::mat4 localBindTransform = glm::mat4(1.0f);
    };

    struct Skeleton {
        std::vector<Bone> bones;

        glm::mat4 globalInverseTransform = glm::mat4(1.0f);

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