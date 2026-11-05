#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <cmath>
#include <algorithm>

namespace qivforge::terrain {

enum class VegetationType : uint16_t {
    TREE = 0,
    ROCK = 1,
    GRASS = 2,
    BUSH = 3
};

#pragma pack(push, 1)
struct VegetationInstance {
    uint16_t localX;   // 0..65535 maps to 0..64m
    uint16_t localY;   // 0..65535 maps to -50..450m
    uint16_t localZ;   // 0..65535 maps to 0..64m
    uint8_t  yaw;      // 0..255 maps to 0..360 deg
    uint8_t  scale;    // 0..255 maps to 0.25m..4.0m
    uint16_t itemId;   // VegetationType
};
#pragma pack(pop)

struct VegetationItemConfig {
    VegetationType type = VegetationType::TREE;
    std::string modelAsset;
    float maxDrawDistance = 250.0f; // Grass 60m, Trees 250m, Rocks 200m
    bool hasCollider = true;        // Trees/rocks have colliders, grass never has
};

class ChunkVegetation {
public:
    static constexpr float CHUNK_SIZE = 64.0f;
    static constexpr float MIN_Y = -50.0f;
    static constexpr float MAX_Y = 450.0f;

    ChunkVegetation(int32_t chunkX, int32_t chunkZ);

    [[nodiscard]] int32_t GetChunkX() const noexcept { return m_chunkX; }
    [[nodiscard]] int32_t GetChunkZ() const noexcept { return m_chunkZ; }

    void AddInstance(float localX, float localY, float localZ, float yawDeg, float scale, uint16_t itemId);
    void Clear() noexcept { m_instances.clear(); }
    [[nodiscard]] size_t GetInstanceCount() const noexcept { return m_instances.size(); }
    [[nodiscard]] const std::vector<VegetationInstance>& GetInstances() const noexcept { return m_instances; }

    // Serialization
    [[nodiscard]] std::vector<uint8_t> Serialize() const;
    bool Deserialize(const uint8_t* data, size_t size);

    // Visibility & Culling
    struct VisibleInstance {
        float worldX, worldY, worldZ;
        float yawDeg;
        float scale;
        uint16_t itemId;
    };

    void QueryVisibleInstances(float camX, float camY, float camZ,
                               float densityMultiplier,
                               const std::vector<VegetationItemConfig>& configs,
                               std::vector<VisibleInstance>& outInstances) const;

private:
    int32_t m_chunkX;
    int32_t m_chunkZ;
    std::vector<VegetationInstance> m_instances;
};

class VegetationManager {
public:
    VegetationManager();

    void RegisterItem(const VegetationItemConfig& config);
    [[nodiscard]] const std::vector<VegetationItemConfig>& GetConfigs() const noexcept { return m_configs; }

    std::shared_ptr<ChunkVegetation> GetOrCreateChunk(int32_t cx, int32_t cz);
    [[nodiscard]] size_t GetTotalInstanceCount() const noexcept;

    // Collects all visible instances across all registered chunks
    size_t CullAndCollect(float camX, float camY, float camZ,
                          float densityMultiplier,
                          std::vector<ChunkVegetation::VisibleInstance>& outInstances) const;

private:
    std::vector<VegetationItemConfig> m_configs;
    std::vector<std::shared_ptr<ChunkVegetation>> m_chunks;
};

} // namespace qivforge::terrain
