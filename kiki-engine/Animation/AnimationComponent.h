#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include <volk.h>

// 引入你之前定义的头文件
#include "Animation/Skeleton.h"
#include "Animation/Animation.h"
#include "Animation/Animator.h"
#include "renderer/utils/Buffer.hpp" // 包含 rutils::Buffer

namespace Kiki {

    /**
     * @brief 动画组件
     * 挂载此组件的实体将被视为“骨骼动画实体”
     */
    struct AnimationComponent {
        // --- 逻辑数据 ---
        std::unique_ptr<Skeleton> skeleton;
        std::unique_ptr<Animation> currentAnimation;
        Animator animator;

        // --- GPU 资源 ---

        // 存放最终骨骼矩阵的 UBO
        // 每一帧 Animator 计算完 finalMatrices 后，会 memcpy 到这个 Buffer
        rutils::Buffer boneMatrixBuffer;

        // 该实体独有的 Descriptor Set，用于在渲染时绑定到 Binding 0 (Animation Layout)
        // 注意：这需要从 RenderManager 的 Pool 中分配
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

        // --- 配置参数 ---
        bool isPlaying = true;
        float playbackSpeed = 1.0f;

        // 为了方便管理，建议定义一个最大骨骼数，需与 Shader 保持一致
        static constexpr uint32_t MAX_BONES = 100;

        // 构造函数简化初始化（可选）
        AnimationComponent() = default;

        // 由于包含 unique_ptr 和 rutils::Buffer，建议显式处理移动语义
        AnimationComponent(AnimationComponent&&) noexcept = default;
        AnimationComponent& operator=(AnimationComponent&&) noexcept = default;

        // 禁用拷贝（Buffer 和 unique_ptr 不允许简单拷贝）
        AnimationComponent(const AnimationComponent&) = delete;
        AnimationComponent& operator=(const AnimationComponent&) = delete;

        /**
         * @brief 辅助函数：更新 GPU 缓冲区
         * 在 RenderManager 的每一帧更新逻辑中调用
         */
        void UpdateGpuBuffer(VmaAllocator allocator) {
            if (animator.finalMatrices.empty()) return;

            // 限制拷贝大小，防止超出 Shader 定义的数组边界
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