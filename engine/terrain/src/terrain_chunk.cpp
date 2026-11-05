#include "qivforge/terrain/terrain_chunk.h"
#include <cstring>
#include <cmath>

namespace qivforge::terrain {

TerrainChunk::TerrainChunk(int32_t chunkX, int32_t chunkZ, float minHeight, float maxHeight)
    : m_chunkX(chunkX), m_chunkZ(chunkZ), m_minHeight(minHeight), m_maxHeight(maxHeight) {
    m_heightmap.resize(TOTAL_SAMPLES, MetersToRaw(0.0f)); // Flat zero height by default
    m_splatWeights.resize(SPLAT_RESOLUTION * SPLAT_RESOLUTION);
    for (auto& w : m_splatWeights) {
        w.fill(0);
        w[0] = 255; // Default Layer 0 (e.g. Grass) is 100%
    }
    m_dirtyRect.Reset();
}

uint16_t TerrainChunk::GetRawHeight(int32_t x, int32_t z) const noexcept {
    if (x < 0 || x >= RESOLUTION || z < 0 || z >= RESOLUTION) return MetersToRaw(0.0f);
    return m_heightmap[z * RESOLUTION + x];
}

void TerrainChunk::SetRawHeight(int32_t x, int32_t z, uint16_t rawHeight) noexcept {
    if (x < 0 || x >= RESOLUTION || z < 0 || z >= RESOLUTION) return;
    int32_t idx = z * RESOLUTION + x;
    if (m_heightmap[idx] != rawHeight) {
        m_heightmap[idx] = rawHeight;
        m_dirtyRect.Encapsulate(x, z);
        m_isModified = true;
    }
}

float TerrainChunk::GetHeight(int32_t x, int32_t z) const noexcept {
    return RawToMeters(GetRawHeight(x, z));
}

void TerrainChunk::SetHeight(int32_t x, int32_t z, float heightMeters) noexcept {
    SetRawHeight(x, z, MetersToRaw(heightMeters));
}

float TerrainChunk::SampleHeightInterpolated(float localX, float localZ) const noexcept {
    float u = localX / SAMPLE_SPACING;
    float v = localZ / SAMPLE_SPACING;

    int32_t x0 = std::clamp(static_cast<int32_t>(std::floor(u)), 0, RESOLUTION - 2);
    int32_t z0 = std::clamp(static_cast<int32_t>(std::floor(v)), 0, RESOLUTION - 2);
    int32_t x1 = x0 + 1;
    int32_t z1 = z0 + 1;

    float fx = std::clamp(u - static_cast<float>(x0), 0.0f, 1.0f);
    float fz = std::clamp(v - static_cast<float>(z0), 0.0f, 1.0f);

    float h00 = GetHeight(x0, z0);
    float h10 = GetHeight(x1, z0);
    float h01 = GetHeight(x0, z1);
    float h11 = GetHeight(x1, z1);

    float h0 = h00 * (1.0f - fx) + h10 * fx;
    float h1 = h01 * (1.0f - fx) + h11 * fx;
    return h0 * (1.0f - fz) + h1 * fz;
}

uint8_t TerrainChunk::GetSplatWeight(int32_t layerIdx, int32_t x, int32_t z) const noexcept {
    if (layerIdx < 0 || layerIdx >= MAX_LAYERS || x < 0 || x >= SPLAT_RESOLUTION || z < 0 || z >= SPLAT_RESOLUTION) {
        return 0;
    }
    return m_splatWeights[z * SPLAT_RESOLUTION + x][layerIdx];
}

void TerrainChunk::SetSplatWeight(int32_t layerIdx, int32_t x, int32_t z, uint8_t weight) noexcept {
    if (layerIdx < 0 || layerIdx >= MAX_LAYERS || x < 0 || x >= SPLAT_RESOLUTION || z < 0 || z >= SPLAT_RESOLUTION) {
        return;
    }
    int32_t idx = z * SPLAT_RESOLUTION + x;
    m_splatWeights[idx][layerIdx] = weight;
    m_isModified = true;
}

int32_t TerrainChunk::GetDominantLayer(int32_t x, int32_t z) const noexcept {
    if (x < 0 || x >= SPLAT_RESOLUTION || z < 0 || z >= SPLAT_RESOLUTION) return 0;
    const auto& weights = m_splatWeights[z * SPLAT_RESOLUTION + x];
    int32_t bestIdx = 0;
    uint8_t maxW = 0;
    for (int32_t i = 0; i < MAX_LAYERS; ++i) {
        if (weights[i] > maxW) {
            maxW = weights[i];
            bestIdx = i;
        }
    }
    return bestIdx;
}

void TerrainChunk::SetHeightmapData(const std::vector<uint16_t>& data) {
    if (data.size() == TOTAL_SAMPLES) {
        m_heightmap = data;
        m_dirtyRect.minX = 0; m_dirtyRect.minZ = 0;
        m_dirtyRect.maxX = RESOLUTION - 1; m_dirtyRect.maxZ = RESOLUTION - 1;
        m_isModified = true;
    }
}

std::vector<uint8_t> TerrainChunk::ExportR16() const {
    std::vector<uint8_t> bytes(TOTAL_SAMPLES * sizeof(uint16_t));
    std::memcpy(bytes.data(), m_heightmap.data(), bytes.size());
    return bytes;
}

bool TerrainChunk::ImportR16(const uint8_t* data, size_t size) {
    if (!data || size != TOTAL_SAMPLES * sizeof(uint16_t)) return false;
    std::memcpy(m_heightmap.data(), data, size);
    m_dirtyRect.minX = 0; m_dirtyRect.minZ = 0;
    m_dirtyRect.maxX = RESOLUTION - 1; m_dirtyRect.maxZ = RESOLUTION - 1;
    m_isModified = true;
    return true;
}

std::vector<uint8_t> TerrainChunk::ExportSplatData() const {
    size_t byteCount = m_splatWeights.size() * MAX_LAYERS;
    std::vector<uint8_t> bytes(byteCount);
    std::memcpy(bytes.data(), m_splatWeights.data(), byteCount);
    return bytes;
}

bool TerrainChunk::ImportSplatData(const uint8_t* data, size_t size) {
    size_t expectedSize = m_splatWeights.size() * MAX_LAYERS;
    if (!data || size != expectedSize) return false;
    std::memcpy(m_splatWeights.data(), data, size);
    m_isModified = true;
    return true;
}

void TerrainChunk::GenerateLodMesh(int32_t lodLevel, float skirtDepth,
                                   std::vector<MeshVertex>& outVertices,
                                   std::vector<uint32_t>& outIndices) const {
    outVertices.clear();
    outIndices.clear();

    int32_t step = 1 << std::clamp(lodLevel, 0, 2); // 1, 2, or 4
    int32_t gridWidth = (RESOLUTION - 1) / step + 1;

    outVertices.reserve(gridWidth * gridWidth + gridWidth * 4); // Grid + skirts

    // 1. Generate surface vertices
    for (int32_t z = 0; z < RESOLUTION; z += step) {
        for (int32_t x = 0; x < RESOLUTION; x += step) {
            MeshVertex v;
            v.x = static_cast<float>(x) * SAMPLE_SPACING;
            v.y = GetHeight(x, z);
            v.z = static_cast<float>(z) * SAMPLE_SPACING;

            // Simple central difference normal
            float hL = GetHeight(std::max(0, x - step), z);
            float hR = GetHeight(std::min(RESOLUTION - 1, x + step), z);
            float hD = GetHeight(x, std::max(0, z - step));
            float hU = GetHeight(x, std::min(RESOLUTION - 1, z + step));

            float nx = -(hR - hL) / (2.0f * step * SAMPLE_SPACING);
            float nz = -(hU - hD) / (2.0f * step * SAMPLE_SPACING);
            float ny = 1.0f;
            float len = std::sqrt(nx * nx + ny * ny + nz * nz);
            v.nx = nx / len; v.ny = ny / len; v.nz = nz / len;

            v.u = static_cast<float>(x) / static_cast<float>(RESOLUTION - 1);
            v.v = static_cast<float>(z) / static_cast<float>(RESOLUTION - 1);

            outVertices.push_back(v);
        }
    }

    // Surface index buffer
    for (int32_t gz = 0; gz < gridWidth - 1; ++gz) {
        for (int32_t gx = 0; gx < gridWidth - 1; ++gx) {
            uint32_t tl = gz * gridWidth + gx;
            uint32_t tr = tl + 1;
            uint32_t bl = (gz + 1) * gridWidth + gx;
            uint32_t br = bl + 1;

            outIndices.push_back(tl);
            outIndices.push_back(bl);
            outIndices.push_back(tr);

            outIndices.push_back(tr);
            outIndices.push_back(bl);
            outIndices.push_back(br);
        }
    }

    // 2. Generate skirts along boundaries to avoid cracks (Doc 05 §10 & Doc 10 §A4)
    if (skirtDepth > 0.0f) {
        auto addSkirtEdge = [&](const std::vector<uint32_t>& edgeIndices) {
            for (size_t i = 0; i < edgeIndices.size() - 1; ++i) {
                uint32_t topA = edgeIndices[i];
                uint32_t topB = edgeIndices[i + 1];

                MeshVertex vA = outVertices[topA];
                vA.y -= skirtDepth;
                MeshVertex vB = outVertices[topB];
                vB.y -= skirtDepth;

                uint32_t botA = static_cast<uint32_t>(outVertices.size());
                outVertices.push_back(vA);
                uint32_t botB = static_cast<uint32_t>(outVertices.size());
                outVertices.push_back(vB);

                outIndices.push_back(topA);
                outIndices.push_back(botA);
                outIndices.push_back(topB);

                outIndices.push_back(topB);
                outIndices.push_back(botA);
                outIndices.push_back(botB);
            }
        };

        // North edge (z = 0)
        std::vector<uint32_t> northEdge;
        for (int32_t gx = 0; gx < gridWidth; ++gx) northEdge.push_back(gx);
        addSkirtEdge(northEdge);

        // South edge (z = max)
        std::vector<uint32_t> southEdge;
        for (int32_t gx = 0; gx < gridWidth; ++gx) southEdge.push_back((gridWidth - 1) * gridWidth + gx);
        addSkirtEdge(southEdge);

        // West edge (x = 0)
        std::vector<uint32_t> westEdge;
        for (int32_t gz = 0; gz < gridWidth; ++gz) westEdge.push_back(gz * gridWidth);
        addSkirtEdge(westEdge);

        // East edge (x = max)
        std::vector<uint32_t> eastEdge;
        for (int32_t gz = 0; gz < gridWidth; ++gz) eastEdge.push_back(gz * gridWidth + (gridWidth - 1));
        addSkirtEdge(eastEdge);
    }
}

} // namespace qivforge::terrain
