#include "AnimationLoader.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Kiki {

    static glm::mat4 ToGlmMat4(const aiMatrix4x4& m) {
        return glm::mat4(
            m.a1, m.b1, m.c1, m.d1,
            m.a2, m.b2, m.c2, m.d2,
            m.a3, m.b3, m.c3, m.d3,
            m.a4, m.b4, m.c4, m.d4
        );
    }

    static glm::vec3 ToVec3(const aiVector3D& v) {
        return glm::vec3(v.x, v.y, v.z);
    }

    static glm::quat ToQuat(const aiQuaternion& q) {
        return glm::quat(q.w, q.x, q.y, q.z);
    }

    static void LoadInverseBindMatrices(
        const aiScene* scene,
        Skeleton& skeleton
    ) {
        if (!scene) return;

        for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
            aiMesh* mesh = scene->mMeshes[m];

            for (unsigned int b = 0; b < mesh->mNumBones; b++) {
                aiBone* aiBone = mesh->mBones[b];

                std::string name = aiBone->mName.C_Str();

                int index = skeleton.FindBoneIndex(name);
                if (index == -1) {
                    continue;
                }

                skeleton.bones[index].inverseBind = ToGlmMat4(aiBone->mOffsetMatrix);
            }
        }
    }

    static void ProcessNode(
        aiNode* node,
        int parentIndex,
        Skeleton& skeleton
    ) {
        Bone bone;
        bone.name = node->mName.C_Str();
        bone.parentIndex = parentIndex;
        bone.localBindTransform = ToGlmMat4(node->mTransformation);

        int currentIndex = (int)skeleton.bones.size();
        skeleton.bones.push_back(bone);

        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            ProcessNode(node->mChildren[i], currentIndex, skeleton);
        }
    }

    std::unique_ptr<Skeleton> AnimationLoader::LoadSkeleton(const aiScene* scene) {
        if (!scene || !scene->mRootNode) return nullptr;

        auto skeleton = std::make_unique<Skeleton>();

        skeleton->globalInverseTransform = glm::inverse(ToGlmMat4(scene->mRootNode->mTransformation));

        ProcessNode(scene->mRootNode, -1, *skeleton);

        LoadInverseBindMatrices(scene, *skeleton);

        return skeleton;
    }

    std::unique_ptr<Animation> AnimationLoader::LoadAnimation(
        const aiScene* scene,
        const Skeleton& skeleton,
        int animationIndex
    ) {
        if (!scene || scene->mNumAnimations == 0) return nullptr;

        aiAnimation* anim = scene->mAnimations[animationIndex];

        auto animation = std::make_unique<Animation>();

        // duration(second)
        float ticksPerSecond = (float)(anim->mTicksPerSecond != 0.0 ? anim->mTicksPerSecond : 25.0);
        animation->duration = (float)(anim->mDuration / ticksPerSecond);

        // track num = bone num
        animation->tracks.resize(skeleton.bones.size());

        // read all bone channels
        for (unsigned int i = 0; i < anim->mNumChannels; i++) {
            aiNodeAnim* channel = anim->mChannels[i];

            std::string boneName = channel->mNodeName.C_Str();
            int boneIndex = skeleton.FindBoneIndex(boneName);

            if (boneIndex == -1) {
                continue;
            }

            BoneTrack& track = animation->tracks[boneIndex];

            size_t keyCount = std::max({
                channel->mNumPositionKeys,
                channel->mNumRotationKeys,
                channel->mNumScalingKeys
                });

            track.keyframes.resize(keyCount);

            for (size_t k = 0; k < keyCount; k++) {
                Keyframe& key = track.keyframes[k];

                float time = 0.0f;
                if (k < channel->mNumPositionKeys) {
                    time = (float)(channel->mPositionKeys[k].mTime / ticksPerSecond);
                }
                key.time = time;

                if (k < channel->mNumPositionKeys) {
                    key.translation = ToVec3(channel->mPositionKeys[k].mValue);
                }

                if (k < channel->mNumRotationKeys) {
                    key.rotation = ToQuat(channel->mRotationKeys[k].mValue);
                }

                if (k < channel->mNumScalingKeys) {
                    key.scale = ToVec3(channel->mScalingKeys[k].mValue);
                }
            }
        }

        return animation;
    }
}