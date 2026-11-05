#include "qivforge/terrain/navmesh_baker.h"
#include <cstring>
#include <cmath>

namespace qivforge::terrain {

ChunkNavmesh::ChunkNavmesh(int32_t chunkX, int32_t chunkZ)
    : m_chunkX(chunkX), m_chunkZ(chunkZ) {
    m_walkableGrid.resize(m_gridDim * m_gridDim, 1);
}

void ChunkNavmesh::SetWalkableGrid(int32_t gridDim, const std::vector<uint8_t>& walkable) {
    m_gridDim = gridDim;
    m_walkableGrid = walkable;
}

bool ChunkNavmesh::IsCellWalkable(int32_t gx, int32_t gz) const noexcept {
    if (gx < 0 || gx >= m_gridDim || gz < 0 || gz >= m_gridDim) return false;
    return m_walkableGrid[gz * m_gridDim + gx] != 0;
}

bool ChunkNavmesh::IsPointWalkable(float localX, float localZ) const noexcept {
    float cellSize = TerrainChunk::CHUNK_SIZE_METERS / static_cast<float>(m_gridDim);
    int32_t gx = static_cast<int32_t>(localX / cellSize);
    int32_t gz = static_cast<int32_t>(localZ / cellSize);
    return IsCellWalkable(gx, gz);
}

std::vector<uint8_t> ChunkNavmesh::Serialize() const {
    size_t headerSize = sizeof(int32_t) * 3; // chunkX, chunkZ, gridDim
    size_t gridSize = m_walkableGrid.size();
    size_t polyCount = m_polygons.size();

    std::vector<uint8_t> data(headerSize + gridSize + sizeof(uint32_t));
    uint8_t* ptr = data.data();

    std::memcpy(ptr, &m_chunkX, sizeof(int32_t)); ptr += sizeof(int32_t);
    std::memcpy(ptr, &m_chunkZ, sizeof(int32_t)); ptr += sizeof(int32_t);
    std::memcpy(ptr, &m_gridDim, sizeof(int32_t)); ptr += sizeof(int32_t);

    std::memcpy(ptr, m_walkableGrid.data(), gridSize); ptr += gridSize;
    uint32_t pCount = static_cast<uint32_t>(polyCount);
    std::memcpy(ptr, &pCount, sizeof(uint32_t));

    return data;
}

bool ChunkNavmesh::Deserialize(const uint8_t* data, size_t size) {
    size_t minSize = sizeof(int32_t) * 3 + sizeof(uint32_t);
    if (!data || size < minSize) return false;

    const uint8_t* ptr = data;
    std::memcpy(&m_chunkX, ptr, sizeof(int32_t)); ptr += sizeof(int32_t);
    std::memcpy(&m_chunkZ, ptr, sizeof(int32_t)); ptr += sizeof(int32_t);
    std::memcpy(&m_gridDim, ptr, sizeof(int32_t)); ptr += sizeof(int32_t);

    size_t gridSize = m_gridDim * m_gridDim;
    if (size < sizeof(int32_t) * 3 + gridSize + sizeof(uint32_t)) return false;

    m_walkableGrid.resize(gridSize);
    std::memcpy(m_walkableGrid.data(), ptr, gridSize); ptr += gridSize;

    return true;
}

std::shared_ptr<ChunkNavmesh> NavmeshBaker::BakeChunk(const TerrainChunk& chunk, const NavmeshBakeParams& params) {
    auto nav = std::make_shared<ChunkNavmesh>(chunk.GetChunkX(), chunk.GetChunkZ());

    int32_t dim = 64; // 1m per cell for 64m chunk
    float cellSize = TerrainChunk::CHUNK_SIZE_METERS / static_cast<float>(dim);
    std::vector<uint8_t> walkable(dim * dim, 1);

    float maxSlopeRad = params.maxSlopeDeg * (3.14159265f / 180.0f);
    float maxTanSlope = std::tan(maxSlopeRad);

    for (int32_t gz = 0; gz < dim; ++gz) {
        float lz0 = static_cast<float>(gz) * cellSize;
        float lz1 = lz0 + cellSize;
        for (int32_t gx = 0; gx < dim; ++gx) {
            float lx0 = static_cast<float>(gx) * cellSize;
            float lx1 = lx0 + cellSize;

            float h00 = chunk.SampleHeightInterpolated(lx0, lz0);
            float h10 = chunk.SampleHeightInterpolated(lx1, lz0);
            float h01 = chunk.SampleHeightInterpolated(lx0, lz1);
            float h11 = chunk.SampleHeightInterpolated(lx1, lz1);

            float slopeX = std::max(std::abs(h10 - h00), std::abs(h11 - h01)) / cellSize;
            float slopeZ = std::max(std::abs(h01 - h00), std::abs(h11 - h10)) / cellSize;
            float maxSlope = std::max(slopeX, slopeZ);

            if (maxSlope > maxTanSlope) {
                walkable[gz * dim + gx] = 0; // Exceeds max slope
            }
        }
    }

    nav->SetWalkableGrid(dim, walkable);
    return nav;
}

} // namespace qivforge::terrain
