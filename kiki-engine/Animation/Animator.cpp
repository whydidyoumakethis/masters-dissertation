#include "Animator.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <tracy/tracy.hpp>

namespace Kiki
{

    static glm::mat4 ComposeTransform(
        const glm::vec3& t,
        const glm::quat& r,
        const glm::vec3& s)
    {
        glm::mat4 T = glm::translate(glm::mat4(1.0f), t);
        glm::mat4 R = glm::toMat4(r);
        glm::mat4 S = glm::scale(glm::mat4(1.0f), s);
        return T * R * S;
    }

    static Keyframe SampleTrack(const BoneTrack& track, float time)
    {
        if (track.keyframes.empty()) {
            return {};
        }

        for (int i = (int)track.keyframes.size() - 1; i >= 0; i--) {
            if (time >= track.keyframes[i].time) {
                return track.keyframes[i];
            }
        }

        return track.keyframes[0];
    }

    void Animator::Update(float dt, const Skeleton& skeleton, const Animation& animation)
    {
        ZoneScopedN("Animation update");

        if (animation.duration <= 0.0f) return;


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

        // caculate localMatrices ---
        for (size_t i = 0; i < boneCount; i++) {
            const BoneTrack& track = animation.tracks[i];

            if (track.keyframes.empty()) {
                // no animation → use bind pose
                localMatrices[i] = skeleton.bones[i].localBindTransform;
            }
            else {
                Keyframe key = SampleTrack(track, currentTime);

                localMatrices[i] = ComposeTransform(
                    key.translation,
                    key.rotation,
                    key.scale
                );
            }
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
            finalMatrices[i] = skeleton.globalInverseTransform * globalMatrices[i] * skeleton.bones[i].inverseBind;
        }
    }

    void Animator::UpdateBlended(float dt, float& prevTime, const Skeleton& skeleton,
        const Animation& prevAnim, const Animation& currentAnim,
        float blendFactor)
    {
        currentTime += dt;
        prevTime += dt;

		ZoneScopedN("Animation blended update");

        if (looping) {
            while (currentTime > currentAnim.duration && currentAnim.duration > 0) currentTime -= currentAnim.duration;
        }
        else {
            if (currentTime > currentAnim.duration) currentTime = currentAnim.duration;
        }

        while (prevTime > prevAnim.duration && prevAnim.duration > 0) prevTime -= prevAnim.duration;

        size_t boneCount = skeleton.bones.size();
        localMatrices.resize(boneCount);
        globalMatrices.resize(boneCount);
        finalMatrices.resize(boneCount);

        // --- 1. caculate mixed localMatrices ---
        for (size_t i = 0; i < boneCount; i++) {
            const BoneTrack& prevTrack = prevAnim.tracks[i];
            const BoneTrack& currTrack = currentAnim.tracks[i];

            // get old animation's SRT
            glm::vec3 pTrans = glm::vec3(0.0f);
            glm::quat pRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            glm::vec3 pScale = glm::vec3(1.0f);

            if (!prevTrack.keyframes.empty()) {
                Keyframe pk = SampleTrack(prevTrack, prevTime);
                pTrans = pk.translation; pRot = pk.rotation; pScale = pk.scale;
            }

            // get new animation's SRT
            glm::vec3 cTrans = glm::vec3(0.0f);
            glm::quat cRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            glm::vec3 cScale = glm::vec3(1.0f);

            if (!currTrack.keyframes.empty()) {
                Keyframe ck = SampleTrack(currTrack, currentTime);
                cTrans = ck.translation; cRot = ck.rotation; cScale = ck.scale;
            }

            if (prevTrack.keyframes.empty() && currTrack.keyframes.empty()) {
                localMatrices[i] = skeleton.bones[i].localBindTransform;
            }
            else {
                // maths thing about blending......
                glm::vec3 blendedTrans = glm::mix(pTrans, cTrans, blendFactor);
                glm::vec3 blendedScale = glm::mix(pScale, cScale, blendFactor);
                glm::quat blendedRot = glm::slerp(pRot, cRot, blendFactor);
                blendedRot = glm::normalize(blendedRot); 

				// new mixed local matrix
                localMatrices[i] = ComposeTransform(blendedTrans, blendedRot, blendedScale);
            }
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
            finalMatrices[i] = skeleton.globalInverseTransform * globalMatrices[i] * skeleton.bones[i].inverseBind;
        }
    }
}