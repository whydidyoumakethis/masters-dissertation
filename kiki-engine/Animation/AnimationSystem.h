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
                auto currentIt = animComp.animations.find(animComp.currentState);

                if (currentIt != animComp.animations.end() && currentIt->second) {
                    Kiki::Animation* currentAnim = currentIt->second.get();

                    // if need blending
                    if (animComp.isBlending) {
                        animComp.blendTimer += dt * animComp.playbackSpeed;
                        float blendFactor = animComp.blendTimer / animComp.blendDuration;

                        if (blendFactor >= 1.0f) {
                            animComp.isBlending = false;
                            animComp.animator.Update(dt * animComp.playbackSpeed, *animComp.skeleton, *currentAnim);
                        }
                        else {
                            auto prevIt = animComp.animations.find(animComp.previousState);
                            if (prevIt != animComp.animations.end() && prevIt->second) {
                                Kiki::Animation* prevAnim = prevIt->second.get();
                                animComp.animator.UpdateBlended(
                                    dt * animComp.playbackSpeed,
                                    animComp.previousPlaybackTime,
                                    *animComp.skeleton,
                                    *prevAnim,
                                    *currentAnim,
                                    blendFactor
                                );
                            }
                            else {
                                // No pervious animation, no blending
                                animComp.isBlending = false;
                                animComp.animator.Update(dt * animComp.playbackSpeed, *animComp.skeleton, *currentAnim);
                            }
                        }
                    }
                    else {
                        animComp.animator.Update(dt * animComp.playbackSpeed, *animComp.skeleton, *currentAnim);
                    }

                    animComp.UpdateGpuBuffer(Kiki::RenderManager::get().allocator.allocator);
                }
            }
        }
    }
};