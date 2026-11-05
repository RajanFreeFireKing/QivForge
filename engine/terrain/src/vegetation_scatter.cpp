#include "qivforge/terrain/vegetation_scatter.h"
#include <cstring>
#include <cmath>

namespace qivforge::terrain {

ChunkVegetation::ChunkVegetation(int32_t chunkX, int32_t chunkZ)
    : m_chunkX(chunkX), m_chunkZ(chunkZ) {}

void ChunkVegetation::AddInstance(float localX, float localY, float localZ, float yawDeg, float scale, uint16_t itemId) {
    VegetationInstance inst;
    inst.localX = static_cast<uint16_t>(std::clamp(localX / CHUNK_SIZE, 0.0f, 1.0f) * 65535.0f);
    inst.localY = static_cast<uint16_t>(std::clamp((localY - MIN_Y) / (MAX_Y - MIN_Y), 0.0f, 1.0f) * 65535.0f);
    inst.localZ = static_cast<uint16_t>(std::clamp(localZ / CHUNK_SIZE, 0.0f, 1.0f) * 65535.0f);

    float normYaw = std::fmod(yawDeg, 360.0f);
    if (normYaw < 0.0f) normYaw += 360.0f;
    inst.yaw = static_cast<uint8_t>(normYaw / 360.0f * 255.0f);

    float normScale = std::clamp((scale - 0.25f) / 3.75f, 0.0f, 1.0f);
    inst.scale = static_cast<uint8_t>(normScale * 255.0f);
    inst.itemId = itemId;

    m_instances.push_back(inst);
}

std::vector<uint8_t> ChunkVegetation::Serialize() const {
    size_t byteCount = m_instances.size() * sizeof(VegetationInstance);
    std::vector<uint8_t> data(byteCount);
    std::memcpy(data.data(), m_instances.data(), byteCount);
    return data;
}

bool ChunkVegetation::Deserialize(const uint8_t* data, size_t size) {
    if (!data || (size % sizeof(VegetationInstance)) != 0) return false;
    size_t count = size / sizeof(VegetationInstance);
    m_instances.resize(count);
    std::memcpy(m_instances.data(), data, size);
    return true;
}

void ChunkVegetation::QueryVisibleInstances(float camX, float camY, float camZ,
                                            float densityMultiplier,
                                            const std::vector<VegetationItemConfig>& configs,
                                            std::vector<VisibleInstance>& outInstances) const {
    float originX = static_cast<float>(m_chunkX) * CHUNK_SIZE;
    float originZ = static_cast<float>(m_chunkZ) * CHUNK_SIZE;

    // Chunk bounding box check
    float chunkCenterX = originX + CHUNK_SIZE * 0.5f;
    float chunkCenterZ = originZ + CHUNK_SIZE * 0.5f;
    float dx = chunkCenterX - camX;
    float dz = chunkCenterZ - camZ;
    float distToChunk = std::sqrt(dx * dx + dz * dz);

    // If chunk center is further than 350m, all vegetation is culled
    if (distToChunk > 350.0f) return;

    size_t maxToProcess = static_cast<size_t>(m_instances.size() * std::clamp(densityMultiplier, 0.0f, 1.0f));

    for (size_t i = 0; i < maxToProcess; ++i) {
        const auto& inst = m_instances[i];
        float maxDist = 250.0f;
        if (inst.itemId < configs.size()) {
            maxDist = configs[inst.itemId].maxDrawDistance;
        }

        float wx = originX + (static_cast<float>(inst.localX) / 65535.0f) * CHUNK_SIZE;
        float wy = MIN_Y + (static_cast<float>(inst.localY) / 65535.0f) * (MAX_Y - MIN_Y);
        float wz = originZ + (static_cast<float>(inst.localZ) / 65535.0f) * CHUNK_SIZE;

        float pDx = wx - camX;
        float pDy = wy - camY;
        float pDz = wz - camZ;
        float pDistSq = pDx * pDx + pDy * pDy + pDz * pDz;

        if (pDistSq <= (maxDist * maxDist)) {
            VisibleInstance vis;
            vis.worldX = wx;
            vis.worldY = wy;
            vis.worldZ = wz;
            vis.yawDeg = (static_cast<float>(inst.yaw) / 255.0f) * 360.0f;
            vis.scale = 0.25f + (static_cast<float>(inst.scale) / 255.0f) * 3.75f;
            vis.itemId = inst.itemId;
            outInstances.push_back(vis);
        }
    }
}

VegetationManager::VegetationManager() {
    // Default configs per Doc 10 §A5
    VegetationItemConfig treeConfig;
    treeConfig.type = VegetationType::TREE;
    treeConfig.modelAsset = "models/tree.glb";
    treeConfig.maxDrawDistance = 250.0f;
    treeConfig.hasCollider = true;
    RegisterItem(treeConfig);

    VegetationItemConfig rockConfig;
    rockConfig.type = VegetationType::ROCK;
    rockConfig.modelAsset = "models/rock.glb";
    rockConfig.maxDrawDistance = 200.0f;
    rockConfig.hasCollider = true;
    RegisterItem(rockConfig);

    VegetationItemConfig grassConfig;
    grassConfig.type = VegetationType::GRASS;
    grassConfig.modelAsset = "models/grass.glb";
    grassConfig.maxDrawDistance = 60.0f;
    grassConfig.hasCollider = false;
    RegisterItem(grassConfig);
}

void VegetationManager::RegisterItem(const VegetationItemConfig& config) {
    m_configs.push_back(config);
}

std::shared_ptr<ChunkVegetation> VegetationManager::GetOrCreateChunk(int32_t cx, int32_t cz) {
    for (auto& c : m_chunks) {
        if (c->GetChunkX() == cx && c->GetChunkZ() == cz) {
            return c;
        }
    }
    auto chunk = std::make_shared<ChunkVegetation>(cx, cz);
    m_chunks.push_back(chunk);
    return chunk;
}

size_t VegetationManager::GetTotalInstanceCount() const noexcept {
    size_t count = 0;
    for (const auto& c : m_chunks) {
        count += c->GetInstanceCount();
    }
    return count;
}

size_t VegetationManager::CullAndCollect(float camX, float camY, float camZ,
                                        float densityMultiplier,
                                        std::vector<ChunkVegetation::VisibleInstance>& outInstances) const {
    outInstances.clear();
    for (const auto& c : m_chunks) {
        c->QueryVisibleInstances(camX, camY, camZ, densityMultiplier, m_configs, outInstances);
    }
    return outInstances.size();
}

} // namespace qivforge::terrain
