#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <cstdint>

namespace qivforge::anim {

enum class HumanoidRole {
    HIPS,
    SPINE,
    CHEST,
    NECK,
    HEAD,
    LEFT_SHOULDER,
    RIGHT_SHOULDER,
    LEFT_UPPER_ARM,
    RIGHT_UPPER_ARM,
    LEFT_LOWER_ARM,
    RIGHT_LOWER_ARM,
    LEFT_HAND,
    RIGHT_HAND,
    LEFT_UPPER_LEG,
    RIGHT_UPPER_LEG,
    LEFT_LOWER_LEG,
    RIGHT_LOWER_LEG,
    LEFT_FOOT,
    RIGHT_FOOT,
    UNKNOWN
};

enum class MaskPreset {
    FULL_BODY,
    UPPER_BODY,
    LOWER_BODY,
    ARMS,
    HEAD
};

class AvatarMask {
public:
    AvatarMask() = default;
    explicit AvatarMask(MaskPreset preset);

    void SetBoneIncluded(int32_t boneIndex, bool included);
    [[nodiscard]] bool IsBoneIncluded(int32_t boneIndex) const noexcept;

    void SetBoneRole(int32_t boneIndex, HumanoidRole role);
    [[nodiscard]] HumanoidRole GetBoneRole(int32_t boneIndex) const noexcept;

    void ApplyPreset(MaskPreset preset, size_t boneCount);

private:
    std::unordered_set<int32_t> m_includedBones;
    std::vector<HumanoidRole> m_boneRoles;
};

} // namespace qivforge::anim
