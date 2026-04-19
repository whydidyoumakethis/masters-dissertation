#include "ECS/System.h"
#include "Animation/AnimationComponent.h"
#include "Animation/Animator.h"
#include "renderer/RenderManager.hpp"
#include "ECS/World.h"

class AnimationSystem : public System {
public:
    Phase GetPhase() const override {
        // 放在普通的 Update 阶段，确保在 Render 之前完成即可
        return Phase::Update;
    }

    void OnUpdate(float dt) override {
        // 查询所有挂载了 AnimationComponent 的实体
        auto view = World::Get().Query<Kiki::AnimationComponent>();

        for (auto [entity, animComp] : view.each()) {
            // 如果动画准备就绪且正在播放
            if (animComp.isPlaying && animComp.skeleton && animComp.currentAnimation) {

                // 1. 推进时间，重新计算局部到全局的骨骼矩阵
                animComp.animator.Update(dt * animComp.playbackSpeed, *animComp.skeleton, *animComp.currentAnimation);

                // 2. 将计算好的最新一帧矩阵数据，Memcpy 到 GPU 的 UBO 中
                animComp.UpdateGpuBuffer(Kiki::RenderManager::get().allocator.allocator);
            }
        }
    }
};