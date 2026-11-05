#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <array>

namespace qivforge::terrain {

enum class SurfaceType : uint8_t {
    GRASS = 0,
    DIRT = 1,
    MUD = 2,
    ASPHALT = 3,
    SAND = 4,
    ICE = 5
};

struct TerrainLayer {
    std::string name;
    SurfaceType surfaceType = SurfaceType::GRASS;
    float uvTiling = 10.0f;
    std::string albedoAsset;
    std::string normalAsset;
    std::string footstepSound;
};

struct DirtyRect {
    int32_t minX = 0;
    int32_t minZ = 0;
    int32_t maxX = 0;
    int32_t maxZ = 0;

    [[nodiscard]] bool IsValid() const noexcept {
        return minX <= maxX && minZ <= maxZ;
    }

    void Reset() noexcept {
        minX = 10000; minZ = 10000;
        maxX = -10000; maxZ = -10000;
    }

    void Encapsulate(int32_t x, int32_t z) noexcept {
        minX = std::min(minX, x);
        minZ = std::min(minZ, z);
        maxX = std::max(maxX, x);
        maxZ = std::max(maxZ, z);
    }
};

class TerrainChunk {
public:
    static constexpr int32_t RESOLUTION = 129;
    static constexpr int32_t TOTAL_SAMPLES = RESOLUTION * RESOLUTION;
    static constexpr float CHUNK_SIZE_METERS = 64.0f;
    static constexpr float SAMPLE_SPACING = CHUNK_SIZE_METERS / (RESOLUTION - 1); // 0.5m
    static constexpr int32_t SPLAT_RESOLUTION = 128;
    static constexpr int32_t MAX_LAYERS = 8;

    TerrainChunk(int32_t chunkX, int32_t chunkZ, float minHeight = -50.0f, float maxHeight = 450.0f);

    [[nodiscard]] int32_t GetChunkX() const noexcept { return m_chunkX; }
    [[nodiscard]] int32_t GetChunkZ() const noexcept { return m_chunkZ; }
    [[nodiscard]] float GetMinHeight() const noexcept { return m_minHeight; }
    [[nodiscard]] float GetMaxHeight() const noexcept { return m_maxHeight; }

    [[nodiscard]] bool IsModified() const noexcept { return m_isModified; }
    void SetModified(bool modified) noexcept { m_isModified = modified; }

    // Height access
    [[nodiscard]] uint16_t GetRawHeight(int32_t x, int32_t z) const noexcept;
    void SetRawHeight(int32_t x, int32_t z, uint16_t rawHeight) noexcept;

    [[nodiscard]] float GetHeight(int32_t x, int32_t z) const noexcept;
    void SetHeight(int32_t x, int32_t z, float heightMeters) noexcept;

    [[nodiscard]] float SampleHeightInterpolated(float localX, float localZ) const noexcept;

    // Splat map weight access (0 to 7)
    [[nodiscard]] uint8_t GetSplatWeight(int32_t layerIdx, int32_t x, int32_t z) const noexcept;
    void SetSplatWeight(int32_t layerIdx, int32_t x, int32_t z, uint8_t weight) noexcept;
    [[nodiscard]] int32_t GetDominantLayer(int32_t x, int32_t z) const noexcept;

    // Raw buffers
    [[nodiscard]] const std::vector<uint16_t>& GetHeightmap() const noexcept { return m_heightmap; }
    void SetHeightmapData(const std::vector<uint16_t>& data);

    // Dirty Rect tracking
    [[nodiscard]] const DirtyRect& GetDirtyRect() const noexcept { return m_dirtyRect; }
    void ClearDirtyRect() noexcept { m_dirtyRect.Reset(); }

    // Disk serialization
    [[nodiscard]] std::vector<uint8_t> ExportR16() const;
    bool ImportR16(const uint8_t* data, size_t size);

    [[nodiscard]] std::vector<uint8_t> ExportSplatData() const;
    bool ImportSplatData(const uint8_t* data, size_t size);

    // LOD mesh generation with skirts (Doc 05 §10)
    // LOD 0 = step 1 (129x129), LOD 1 = step 2 (65x65), LOD 2 = step 4 (33x33)
    struct MeshVertex {
        float x, y, z;
        float nx, ny, nz;
        float u, v;
    };
    void GenerateLodMesh(int32_t lodLevel, float skirtDepth,
                         std::vector<MeshVertex>& outVertices,
                         std::vector<uint32_t>& outIndices) const;

private:
    int32_t m_chunkX;
    int32_t m_chunkZ;
    float m_minHeight;
    float m_maxHeight;
    bool m_isModified = false;

    std::vector<uint16_t> m_heightmap;
    // 2 splat maps x 4 channels x 128 x 128 = 8 layers
    std::vector<std::array<uint8_t, MAX_LAYERS>> m_splatWeights;
    DirtyRect m_dirtyRect;

    [[nodiscard]] float RawToMeters(uint16_t raw) const noexcept {
        return m_minHeight + (static_cast<float>(raw) / 65535.0f) * (m_maxHeight - m_minHeight);
    }

    [[nodiscard]] uint16_t MetersToRaw(float meters) const noexcept {
        float t = (meters - m_minHeight) / (m_maxHeight - m_minHeight);
        t = std::clamp(t, 0.0f, 1.0f);
        return static_cast<uint16_t>(t * 65535.0f + 0.5f);
    }
};

} // namespace qivforge::terrain
