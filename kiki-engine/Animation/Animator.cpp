#include "Animator.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Kiki {

    static glm::mat4 ComposeTransform(
        const glm::vec3& t,
        const glm::quat& r,
        const glm::vec3& s
    ) {
        glm::mat4 T = glm::translate(glm::mat4(1.0f), t);
        glm::mat4 R = glm::toMat4(r);
        glm::mat4 S = glm::scale(glm::mat4(1.0f), s);
        return T * R * S;
    }

    static Keyframe SampleTrack(const BoneTrack& track, float time) {
        if (track.keyframes.empty()) {
            return {};
        }

        // 简化版：直接取最近关键帧（后面可以升级插值）
        for (int i = (int)track.keyframes.size() - 1; i >= 0; i--) {
            if (time >= track.keyframes[i].time) {
                return track.keyframes[i];
            }
        }

        return track.keyframes[0];
    }

    void Animator::Update(float dt, const Skeleton& skeleton, const Animation& animation) {
        if (animation.duration <= 0.0f) return;

        // --- 时间推进 ---
        currentTime += dt;

        if (looping) {
            while (currentTime > animation.duration) {
                currentTime -= animation.duration;
            }
        }
        else {
            if (currentTime > animation.duration) {
                currentTime = animation.duration;
            }
        }

        size_t boneCount = skeleton.bones.size();

        localMatrices.resize(boneCount);
        globalMatrices.resize(boneCount);
        finalMatrices.resize(boneCount);

        // --- 1. 计算 localMatrices ---
        for (size_t i = 0; i < boneCount; i++) {
            const BoneTrack& track = animation.tracks[i];

            Keyframe key = SampleTrack(track, currentTime);

            localMatrices[i] = ComposeTransform(
                key.translation,
                key.rotation,
                key.scale
            );
        }

        // --- 2. local → global ---
        for (size_t i = 0; i < boneCount; i++) {
            int parent = skeleton.bones[i].parentIndex;

            if (parent < 0) {
                globalMatrices[i] = localMatrices[i];
            }
            else {
                globalMatrices[i] = globalMatrices[parent] * localMatrices[i];
            }
        }

        // --- 3. global * inverseBind ---
        for (size_t i = 0; i < boneCount; i++) {
            finalMatrices[i] = globalMatrices[i] * skeleton.bones[i].inverseBind;
        }
    }

}