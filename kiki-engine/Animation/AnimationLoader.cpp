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

        // 遍历所有 mesh
        for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
            aiMesh* mesh = scene->mMeshes[m];

            // 遍历所有 bone
            for (unsigned int b = 0; b < mesh->mNumBones; b++) {
                aiBone* aiBone = mesh->mBones[b];

                std::string name = aiBone->mName.C_Str();

                int index = skeleton.FindBoneIndex(name);
                if (index == -1) {
                    // 找不到就跳过（有些骨头不在节点树里）
                    continue;
                }

                skeleton.bones[index].inverseBind = ToGlmMat4(aiBone->mOffsetMatrix);
            }
        }
    }

    // 递归函数：遍历 aiNode 树
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

        // 递归处理子节点
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            ProcessNode(node->mChildren[i], currentIndex, skeleton);
        }
    }

    std::unique_ptr<Skeleton> AnimationLoader::LoadSkeleton(const aiScene* scene) {
        if (!scene || !scene->mRootNode) return nullptr;

        auto skeleton = std::make_unique<Skeleton>();

        // 从 root 开始递归
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

        // duration（秒）
        float ticksPerSecond = (float)(anim->mTicksPerSecond != 0.0 ? anim->mTicksPerSecond : 25.0);
        animation->duration = (float)(anim->mDuration / ticksPerSecond);

        //关键：track 数量 = bone 数量
        animation->tracks.resize(skeleton.bones.size());

        // 遍历所有 channel（骨头轨道）
        for (unsigned int i = 0; i < anim->mNumChannels; i++) {
            aiNodeAnim* channel = anim->mChannels[i];

            std::string boneName = channel->mNodeName.C_Str();
            int boneIndex = skeleton.FindBoneIndex(boneName);

            if (boneIndex == -1) {
                // 找不到就跳过
                continue;
            }

            BoneTrack& track = animation->tracks[boneIndex];

            // 关键帧数量取三者最大值
            size_t keyCount = std::max({
                channel->mNumPositionKeys,
                channel->mNumRotationKeys,
                channel->mNumScalingKeys
                });

            track.keyframes.resize(keyCount);

            for (size_t k = 0; k < keyCount; k++) {
                Keyframe& key = track.keyframes[k];

                // --- 时间 ---
                float time = 0.0f;
                if (k < channel->mNumPositionKeys) {
                    time = (float)(channel->mPositionKeys[k].mTime / ticksPerSecond);
                }
                key.time = time;

                // --- 平移 ---
                if (k < channel->mNumPositionKeys) {
                    key.translation = ToVec3(channel->mPositionKeys[k].mValue);
                }

                // --- 旋转 ---
                if (k < channel->mNumRotationKeys) {
                    key.rotation = ToQuat(channel->mRotationKeys[k].mValue);
                }

                // --- 缩放 ---
                if (k < channel->mNumScalingKeys) {
                    key.scale = ToVec3(channel->mScalingKeys[k].mValue);
                }
            }
        }

        return animation;
    }
}