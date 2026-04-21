#include "ECS/System.h"
#include "Animation/AnimationComponent.h"
#include "Animation/Animator.h"
#include "renderer/RenderManager.hpp"
#include "ECS/World.h"
#include "../../games/demo/component/CharacterComponent.h"
#include "Animation.h"

class AnimationSystem : public System {
public:
    Phase GetPhase() const override {
        return Phase::Update;
    }

    void OnUpdate(float dt) override {
        auto view = World::Get().Query<CharacterComponent, Kiki::AnimationComponent>();

        for (auto [entity, charComp, animComp] : view.each()) {

            if (animComp.currentState != charComp.state) {
                animComp.ChangeState(charComp.state);
            }

            if (animComp.isPlaying && animComp.skeleton) {

                auto it = animComp.animations.find(animComp.currentState);
                if (it != animComp.animations.end() && it->second) {

                    Kiki::Animation* currentAnim = it->second.get();

                    animComp.animator.Update(dt * animComp.playbackSpeed, *animComp.skeleton, *currentAnim);

                    animComp.UpdateGpuBuffer(Kiki::RenderManager::get().allocator.allocator);
                }
            }
        }
    }
};