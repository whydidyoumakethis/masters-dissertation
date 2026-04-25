#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include <volk.h>
#include <unordered_map>
#include <spdlog/spdlog.h>
#include <string>

#include "Animation/Skeleton.h"
#include "Animation/Animation.h"
#include "Animation/Animator.h"
#include "renderer/utils/Buffer.hpp" 
#include "../../games/demo/component/CharacterComponent.h"

namespace Kiki {

    struct AnimationComponent {
        std::unique_ptr<Skeleton> skeleton;
        //std::unique_ptr<Animation> currentAnimation;
        std::unordered_map<CharacterState, std::unique_ptr<Animation>> animations;
        CharacterState currentState = CharacterState::Idle;
        Animator animator;

        rutils::Buffer boneMatrixBuffer;

        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

        bool isPlaying = true;
        float playbackSpeed = 1.0f;

        static constexpr uint32_t MAX_BONES = 100;

        AnimationComponent() = default;

        AnimationComponent(AnimationComponent&&) noexcept = default;
        AnimationComponent& operator=(AnimationComponent&&) noexcept = default;

        AnimationComponent(const AnimationComponent&) = delete;
        AnimationComponent& operator=(const AnimationComponent&) = delete;

        void ChangeState(CharacterState newState) {
            if (currentState == newState || animations.find(newState) == animations.end()) {
                return;
            }
            
            spdlog::info("[Animation] State Changed: {} -> {}", to_string(currentState), to_string(newState));
            
            currentState = newState;
            animator.currentTime = 0.0f; 

            if (newState == CharacterState::Jumping) {
                animator.looping = false;
            }
            else {
                animator.looping = true;
            }

            

        }

        void UpdateGpuBuffer(VmaAllocator allocator) {
            if (animator.finalMatrices.empty()) return;

            size_t copySize = std::min(
                animator.finalMatrices.size() * sizeof(glm::mat4),
                (size_t)MAX_BONES * sizeof(glm::mat4)
            );

            void* data = nullptr;
            vmaMapMemory(allocator, boneMatrixBuffer.allocation, &data);
            memcpy(data, animator.finalMatrices.data(), copySize);
            vmaUnmapMemory(allocator, boneMatrixBuffer.allocation);
        }
    };

}