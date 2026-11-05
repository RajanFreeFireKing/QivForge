#include "qivforge/anim/avatar_mask.h"

namespace qivforge::anim {

AvatarMask::AvatarMask(MaskPreset preset) {
    // Default constructor with preset will be populated when boneCount is set
    (void)preset;
}

void AvatarMask::SetBoneIncluded(int32_t boneIndex, bool included) {
    if (included) {
        m_includedBones.insert(boneIndex);
    } else {
        m_includedBones.erase(boneIndex);
    }
}

bool AvatarMask::IsBoneIncluded(int32_t boneIndex) const noexcept {
    if (m_includedBones.empty()) return true; // Default full body when empty
    return m_includedBones.find(boneIndex) != m_includedBones.end();
}

void AvatarMask::SetBoneRole(int32_t boneIndex, HumanoidRole role) {
    if (boneIndex < 0) return;
    if (static_cast<size_t>(boneIndex) >= m_boneRoles.size()) {
        m_boneRoles.resize(boneIndex + 1, HumanoidRole::UNKNOWN);
    }
    m_boneRoles[boneIndex] = role;
}

HumanoidRole AvatarMask::GetBoneRole(int32_t boneIndex) const noexcept {
    if (boneIndex >= 0 && static_cast<size_t>(boneIndex) < m_boneRoles.size()) {
        return m_boneRoles[boneIndex];
    }
    return HumanoidRole::UNKNOWN;
}

void AvatarMask::ApplyPreset(MaskPreset preset, size_t boneCount) {
    m_includedBones.clear();
    for (size_t i = 0; i < boneCount; ++i) {
        HumanoidRole role = GetBoneRole(static_cast<int32_t>(i));
        bool include = false;

        switch (preset) {
            case MaskPreset::FULL_BODY:
                include = true;
                break;
            case MaskPreset::UPPER_BODY:
                include = (role == HumanoidRole::SPINE || role == HumanoidRole::CHEST ||
                           role == HumanoidRole::NECK || role == HumanoidRole::HEAD ||
                           role == HumanoidRole::LEFT_SHOULDER || role == HumanoidRole::RIGHT_SHOULDER ||
                           role == HumanoidRole::LEFT_UPPER_ARM || role == HumanoidRole::RIGHT_UPPER_ARM ||
                           role == HumanoidRole::LEFT_LOWER_ARM || role == HumanoidRole::RIGHT_LOWER_ARM ||
                           role == HumanoidRole::LEFT_HAND || role == HumanoidRole::RIGHT_HAND);
                break;
            case MaskPreset::LOWER_BODY:
                include = (role == HumanoidRole::HIPS ||
                           role == HumanoidRole::LEFT_UPPER_LEG || role == HumanoidRole::RIGHT_UPPER_LEG ||
                           role == HumanoidRole::LEFT_LOWER_LEG || role == HumanoidRole::RIGHT_LOWER_LEG ||
                           role == HumanoidRole::LEFT_FOOT || role == HumanoidRole::RIGHT_FOOT);
                break;
            case MaskPreset::ARMS:
                include = (role == HumanoidRole::LEFT_SHOULDER || role == HumanoidRole::RIGHT_SHOULDER ||
                           role == HumanoidRole::LEFT_UPPER_ARM || role == HumanoidRole::RIGHT_UPPER_ARM ||
                           role == HumanoidRole::LEFT_LOWER_ARM || role == HumanoidRole::RIGHT_LOWER_ARM ||
                           role == HumanoidRole::LEFT_HAND || role == HumanoidRole::RIGHT_HAND);
                break;
            case MaskPreset::HEAD:
                include = (role == HumanoidRole::NECK || role == HumanoidRole::HEAD);
                break;
        }

        if (include) {
            m_includedBones.insert(static_cast<int32_t>(i));
        }
    }
}

} // namespace qivforge::anim
