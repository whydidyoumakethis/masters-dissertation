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

namespace Kiki {

    enum class CharacterState {
        Idle,
        Walking,
        Running,
        Jumping,
    };

    inline std::string to_string(CharacterState state) {
        switch (state) {
        case CharacterState::Idle:    return "Idle";
        case CharacterState::Walking: return "Walking";
        case CharacterState::Running: return "Running";
        case CharacterState::Jumping: return "Jumping";
        default:                      return "Unknown";
        }
    }

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

		// for blending
        CharacterState previousState = CharacterState::Idle;
		float blendDuration = 0.2f;    // change this to adjust how long the blending lasts
        float blendTimer = 0.0f;       
        bool isBlending = false;       
        float previousPlaybackTime = 0.0f;

        void ChangeState(CharacterState newState, bool loop = true) {
            if (currentState == newState || animations.find(newState) == animations.end()) {
                return;
            }

            spdlog::info("[Animation] State Changed: {} -> {}", to_string(currentState), to_string(newState));
            
            if (animator.currentTime == 0.0f && !isBlending && previousState == CharacterState::Idle) {
                currentState = newState;
            }
            else {
                previousState = currentState;
                currentState = newState;
                isBlending = true;
                blendTimer = 0.0f;
                previousPlaybackTime = animator.currentTime;
            }

            
            
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