#include "qivforge/anim/skeleton.h"
#include <stdexcept>

namespace qivforge::anim {

int32_t Skeleton::AddBone(const std::string& name, int32_t parentIndex, const BoneTransform& bindPose) {
    if (m_bones.size() >= kMaxBones) {
        return -1; // Limit exceeded
    }

    int32_t index = static_cast<int32_t>(m_bones.size());
    Bone bone;
    bone.name = name;
    bone.parentIndex = parentIndex;
    bone.bindPose = bindPose;
    // Set inverse bind matrix as identity for bind pose base
    bone.inverseBindMatrix = Mat4Identity();

    m_bones.push_back(bone);
    m_boneNameMap[name] = index;
    return index;
}

int32_t Skeleton::FindBoneIndex(const std::string& name) const noexcept {
    auto it = m_boneNameMap.find(name);
    return (it != m_boneNameMap.end()) ? it->second : -1;
}

void Skeleton::ComputeSkinningMatrices(const std::vector<BoneTransform>& localPoses, std::vector<Mat4>& outSkinMatrices) const {
    size_t count = m_bones.size();
    outSkinMatrices.resize(count);
    std::vector<Mat4> worldMatrices(count);

    for (size_t i = 0; i < count; ++i) {
        Mat4 localMat = (i < localPoses.size()) ? BoneTransformToMat4(localPoses[i]) 
                                               : BoneTransformToMat4(m_bones[i].bindPose);

        int32_t parent = m_bones[i].parentIndex;
        if (parent >= 0 && static_cast<size_t>(parent) < count) {
            worldMatrices[i] = Mat4Multiply(worldMatrices[parent], localMat);
        } else {
            worldMatrices[i] = localMat;
        }

        // Final skinning matrix: boneWorldMatrix * inverseBindMatrix
        outSkinMatrices[i] = Mat4Multiply(worldMatrices[i], m_bones[i].inverseBindMatrix);
    }
}

} // namespace qivforge::anim
