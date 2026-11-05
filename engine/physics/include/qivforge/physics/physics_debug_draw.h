#pragma once

#include "qivforge/physics/physics_types.h"
#include <vector>

namespace qivforge::physics {

struct DebugLine {
    Vec3 start;
    Vec3 end;
    uint32_t colorRgba = 0x00FF00FF; // Green default
};

class PhysicsDebugDraw {
public:
    PhysicsDebugDraw() = default;

    void DrawLine(const Vec3& start, const Vec3& end, uint32_t colorRgba = 0x00FF00FF) noexcept {
        m_lines.push_back({start, end, colorRgba});
    }

    void DrawBox(const Vec3& center, const Vec3& halfExtents, uint32_t colorRgba = 0x00FF00FF) noexcept {
        Vec3 min = center - halfExtents;
        Vec3 max = center + halfExtents;

        // 12 edges of a box
        DrawLine({min.x, min.y, min.z}, {max.x, min.y, min.z}, colorRgba);
        DrawLine({max.x, min.y, min.z}, {max.x, max.y, min.z}, colorRgba);
        DrawLine({max.x, max.y, min.z}, {min.x, max.y, min.z}, colorRgba);
        DrawLine({min.x, max.y, min.z}, {min.x, min.y, min.z}, colorRgba);

        DrawLine({min.x, min.y, max.z}, {max.x, min.y, max.z}, colorRgba);
        DrawLine({max.x, min.y, max.z}, {max.x, max.y, max.z}, colorRgba);
        DrawLine({max.x, max.y, max.z}, {min.x, max.y, max.z}, colorRgba);
        DrawLine({min.x, max.y, max.z}, {min.x, min.y, max.z}, colorRgba);

        DrawLine({min.x, min.y, min.z}, {min.x, min.y, max.z}, colorRgba);
        DrawLine({max.x, min.y, min.z}, {max.x, min.y, max.z}, colorRgba);
        DrawLine({max.x, max.y, min.z}, {max.x, max.y, max.z}, colorRgba);
        DrawLine({min.x, max.y, min.z}, {min.x, max.y, max.z}, colorRgba);
    }

    void DrawRay(const Vec3& origin, const Vec3& dir, float length, uint32_t colorRgba = 0xFF0000FF) noexcept {
        DrawLine(origin, origin + dir * length, colorRgba);
    }

    [[nodiscard]] const std::vector<DebugLine>& GetLines() const noexcept { return m_lines; }
    void Clear() noexcept { m_lines.clear(); }

private:
    std::vector<DebugLine> m_lines;
};

} // namespace qivforge::physics
