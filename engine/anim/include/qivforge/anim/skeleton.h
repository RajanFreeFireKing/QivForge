#pragma once

#include "qivforge/anim/anim_math.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace qivforge::anim {

struct Bone {
    std::string name;
    int32_t parentIndex = -1;
    BoneTransform bindPose;
    Mat4 inverseBindMatrix = Mat4Identity();
};

class Skeleton {
public:
    static constexpr size_t kMaxBones = 128; // Doc 05 §9 & Doc 07 §2

    int32_t AddBone(const std::string& name, int32_t parentIndex, const BoneTransform& bindPose);
    [[nodiscard]] int32_t FindBoneIndex(const std::string& name) const noexcept;
    [[nodiscard]] size_t GetBoneCount() const noexcept { return m_bones.size(); }
    [[nodiscard]] const Bone& GetBone(size_t index) const { return m_bones[index]; }
    [[nodiscard]] Bone& GetBone(size_t index) { return m_bones[index]; }

    // Computes final skinning matrices: skinningMatrix = boneWorldMatrix * inverseBindMatrix
    void ComputeSkinningMatrices(const std::vector<BoneTransform>& localPoses, std::vector<Mat4>& outSkinMatrices) const;

private:
    std::vector<Bone> m_bones;
    std::unordered_map<std::string, int32_t> m_boneNameMap;
};

} // namespace qivforge::anim
