#pragma once

#include "qivforge/terrain/terrain_chunk.h"
#include <vector>
#include <cstdint>
#include <memory>

namespace qivforge::terrain {

struct NavmeshBakeParams {
    float agentRadius = 0.35f;
    float agentHeight = 1.8f;
    float maxStepHeight = 0.35f;
    float maxSlopeDeg = 45.0f;
    float cellSize = 1.0f; // 1m voxel grid
};

struct NavPolygon {
    std::vector<float> verticesX;
    std::vector<float> verticesY;
    std::vector<float> verticesZ;
    float centerX = 0.0f;
    float centerY = 0.0f;
    float centerZ = 0.0f;
};

class ChunkNavmesh {
public:
    ChunkNavmesh(int32_t chunkX, int32_t chunkZ);

    [[nodiscard]] int32_t GetChunkX() const noexcept { return m_chunkX; }
    [[nodiscard]] int32_t GetChunkZ() const noexcept { return m_chunkZ; }

    void SetWalkableGrid(int32_t gridDim, const std::vector<uint8_t>& walkable);
    [[nodiscard]] bool IsCellWalkable(int32_t gx, int32_t gz) const noexcept;
    [[nodiscard]] bool IsPointWalkable(float localX, float localZ) const noexcept;

    [[nodiscard]] size_t GetPolygonCount() const noexcept { return m_polygons.size(); }
    void AddPolygon(const NavPolygon& poly) { m_polygons.push_back(poly); }

    // Serialization for navmesh/nav_<x>_<z>.bin
    [[nodiscard]] std::vector<uint8_t> Serialize() const;
    bool Deserialize(const uint8_t* data, size_t size);

private:
    int32_t m_chunkX;
    int32_t m_chunkZ;
    int32_t m_gridDim = 64; // 64x64 for 64m chunk (1m cells)
    std::vector<uint8_t> m_walkableGrid;
    std::vector<NavPolygon> m_polygons;
};

class NavmeshBaker {
public:
    static std::shared_ptr<ChunkNavmesh> BakeChunk(const TerrainChunk& chunk, const NavmeshBakeParams& params);
};

} // namespace qivforge::terrain
