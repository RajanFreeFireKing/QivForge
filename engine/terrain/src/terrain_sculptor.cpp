#include "qivforge/terrain/terrain_sculptor.h"
#include <cmath>
#include <algorithm>

namespace qivforge::terrain {

void TerrainUndoStroke::ApplyUndo(std::map<std::pair<int32_t, int32_t>, std::shared_ptr<TerrainChunk>>& chunks) {
    for (const auto& snap : chunkSnapshots) {
        auto key = std::make_pair(snap.chunkX, snap.chunkZ);
        auto it = chunks.find(key);
        if (it != chunks.end()) {
            if (!snap.beforeHeightmap.empty()) {
                it->second->SetHeightmapData(snap.beforeHeightmap);
            }
            if (!snap.beforeSplat.empty()) {
                it->second->ImportSplatData(snap.beforeSplat.data(), snap.beforeSplat.size());
            }
        }
    }
}

void TerrainUndoStroke::ApplyRedo(std::map<std::pair<int32_t, int32_t>, std::shared_ptr<TerrainChunk>>& chunks) {
    for (const auto& snap : chunkSnapshots) {
        auto key = std::make_pair(snap.chunkX, snap.chunkZ);
        auto it = chunks.find(key);
        if (it != chunks.end()) {
            if (!snap.afterHeightmap.empty()) {
                it->second->SetHeightmapData(snap.afterHeightmap);
            }
            if (!snap.afterSplat.empty()) {
                it->second->ImportSplatData(snap.afterSplat.data(), snap.afterSplat.size());
            }
        }
    }
}

TerrainWorld::TerrainWorld() {
    // Default layers
    m_layers[0].name = "Grass";
    m_layers[0].surfaceType = SurfaceType::GRASS;
    m_layers[1].name = "Mud";
    m_layers[1].surfaceType = SurfaceType::MUD;
    m_layers[2].name = "Dirt";
    m_layers[2].surfaceType = SurfaceType::DIRT;
    m_layers[3].name = "Asphalt";
    m_layers[3].surfaceType = SurfaceType::ASPHALT;
    m_layers[4].name = "Sand";
    m_layers[4].surfaceType = SurfaceType::SAND;
    m_layers[5].name = "Ice";
    m_layers[5].surfaceType = SurfaceType::ICE;
}

std::shared_ptr<TerrainChunk> TerrainWorld::GetOrCreateChunk(int32_t cx, int32_t cz) {
    auto key = std::make_pair(cx, cz);
    auto it = m_chunks.find(key);
    if (it != m_chunks.end()) {
        return it->second;
    }
    auto chunk = std::make_shared<TerrainChunk>(cx, cz);
    m_chunks[key] = chunk;
    return chunk;
}

std::shared_ptr<TerrainChunk> TerrainWorld::GetChunk(int32_t cx, int32_t cz) const {
    auto key = std::make_pair(cx, cz);
    auto it = m_chunks.find(key);
    if (it != m_chunks.end()) {
        return it->second;
    }
    return nullptr;
}

size_t TerrainWorld::GetModifiedChunkCount() const noexcept {
    size_t count = 0;
    for (const auto& [_, chunk] : m_chunks) {
        if (chunk && chunk->IsModified()) {
            count++;
        }
    }
    return count;
}

void TerrainWorld::SetLayer(int32_t index, const TerrainLayer& layer) {
    if (index >= 0 && index < TerrainChunk::MAX_LAYERS) {
        m_layers[index] = layer;
    }
}

const TerrainLayer& TerrainWorld::GetLayer(int32_t index) const {
    static const TerrainLayer s_defaultLayer;
    if (index >= 0 && index < TerrainChunk::MAX_LAYERS) {
        return m_layers[index];
    }
    return s_defaultLayer;
}

float TerrainWorld::GetHeightAt(float worldX, float worldZ) const {
    int32_t cx = static_cast<int32_t>(std::floor(worldX / TerrainChunk::CHUNK_SIZE_METERS));
    int32_t cz = static_cast<int32_t>(std::floor(worldZ / TerrainChunk::CHUNK_SIZE_METERS));

    auto chunk = GetChunk(cx, cz);
    if (!chunk) return 0.0f;

    float localX = worldX - static_cast<float>(cx) * TerrainChunk::CHUNK_SIZE_METERS;
    float localZ = worldZ - static_cast<float>(cz) * TerrainChunk::CHUNK_SIZE_METERS;
    return chunk->SampleHeightInterpolated(localX, localZ);
}

SurfaceType TerrainWorld::GetSurfaceTypeAt(float worldX, float worldZ) const {
    int32_t cx = static_cast<int32_t>(std::floor(worldX / TerrainChunk::CHUNK_SIZE_METERS));
    int32_t cz = static_cast<int32_t>(std::floor(worldZ / TerrainChunk::CHUNK_SIZE_METERS));

    auto chunk = GetChunk(cx, cz);
    if (!chunk) return SurfaceType::GRASS;

    float localX = worldX - static_cast<float>(cx) * TerrainChunk::CHUNK_SIZE_METERS;
    float localZ = worldZ - static_cast<float>(cz) * TerrainChunk::CHUNK_SIZE_METERS;

    int32_t sx = std::clamp(static_cast<int32_t>(localX / TerrainChunk::CHUNK_SIZE_METERS * TerrainChunk::SPLAT_RESOLUTION), 0, TerrainChunk::SPLAT_RESOLUTION - 1);
    int32_t sz = std::clamp(static_cast<int32_t>(localZ / TerrainChunk::CHUNK_SIZE_METERS * TerrainChunk::SPLAT_RESOLUTION), 0, TerrainChunk::SPLAT_RESOLUTION - 1);

    int32_t layerIdx = chunk->GetDominantLayer(sx, sz);
    return GetLayer(layerIdx).surfaceType;
}

float TerrainWorld::CalculateFalloff(float dist, float radius, FalloffType type) const noexcept {
    if (dist >= radius || radius <= 0.0f) return 0.0f;
    float t = dist / radius;
    switch (type) {
        case FalloffType::SMOOTH:
            return 1.0f - (3.0f * t * t - 2.0f * t * t * t);
        case FalloffType::LINEAR:
            return 1.0f - t;
        case FalloffType::SHARP:
            return 1.0f - (t * t);
        default:
            return 1.0f - t;
    }
}

void TerrainWorld::BeginStroke() {
    m_inStroke = true;
    m_strokeInitialHeights.clear();
    m_strokeInitialSplats.clear();
}

void TerrainWorld::ApplyBrush(float worldX, float worldZ, const BrushSettings& brush, float deltaTime) {
    if (!m_inStroke) {
        BeginStroke();
    }

    float r = brush.radiusMeters;
    int32_t minCx = static_cast<int32_t>(std::floor((worldX - r) / TerrainChunk::CHUNK_SIZE_METERS));
    int32_t maxCx = static_cast<int32_t>(std::floor((worldX + r) / TerrainChunk::CHUNK_SIZE_METERS));
    int32_t minCz = static_cast<int32_t>(std::floor((worldZ - r) / TerrainChunk::CHUNK_SIZE_METERS));
    int32_t maxCz = static_cast<int32_t>(std::floor((worldZ + r) / TerrainChunk::CHUNK_SIZE_METERS));

    for (int32_t cz = minCz; cz <= maxCz; ++cz) {
        for (int32_t cx = minCx; cx <= maxCx; ++cx) {
            auto chunk = GetOrCreateChunk(cx, cz);

            // Record initial state for undo if not yet recorded
            auto key = std::make_pair(cx, cz);
            if (m_strokeInitialHeights.find(key) == m_strokeInitialHeights.end()) {
                m_strokeInitialHeights[key] = chunk->GetHeightmap();
                m_strokeInitialSplats[key] = chunk->ExportSplatData();
            }

            float chunkOriginX = static_cast<float>(cx) * TerrainChunk::CHUNK_SIZE_METERS;
            float chunkOriginZ = static_cast<float>(cz) * TerrainChunk::CHUNK_SIZE_METERS;

            if (brush.tool == SculptTool::PAINT_LAYER) {
                // Paint splat layer with proportional normalization across layers
                for (int32_t sz = 0; sz < TerrainChunk::SPLAT_RESOLUTION; ++sz) {
                    float pz = chunkOriginZ + (static_cast<float>(sz) + 0.5f) / TerrainChunk::SPLAT_RESOLUTION * TerrainChunk::CHUNK_SIZE_METERS;
                    for (int32_t sx = 0; sx < TerrainChunk::SPLAT_RESOLUTION; ++sx) {
                        float px = chunkOriginX + (static_cast<float>(sx) + 0.5f) / TerrainChunk::SPLAT_RESOLUTION * TerrainChunk::CHUNK_SIZE_METERS;
                        float dist = std::sqrt((px - worldX) * (px - worldX) + (pz - worldZ) * (pz - worldZ));
                        if (dist <= r) {
                            float f = CalculateFalloff(dist, r, brush.falloff);
                            float blend = std::clamp(brush.strength * f * (deltaTime / 0.016f), 0.0f, 1.0f);
                            for (int32_t l = 0; l < TerrainChunk::MAX_LAYERS; ++l) {
                                float curW = static_cast<float>(chunk->GetSplatWeight(l, sx, sz));
                                if (l == brush.targetLayer) {
                                    float newW = curW + blend * (255.0f - curW);
                                    chunk->SetSplatWeight(l, sx, sz, static_cast<uint8_t>(std::clamp(newW, 0.0f, 255.0f)));
                                } else {
                                    float newW = curW * (1.0f - blend);
                                    chunk->SetSplatWeight(l, sx, sz, static_cast<uint8_t>(std::clamp(newW, 0.0f, 255.0f)));
                                }
                            }
                        }
                    }
                }
            } else {
                // Height sculpting
                for (int32_t z = 0; z < TerrainChunk::RESOLUTION; ++z) {
                    float pz = chunkOriginZ + static_cast<float>(z) * TerrainChunk::SAMPLE_SPACING;
                    for (int32_t x = 0; x < TerrainChunk::RESOLUTION; ++x) {
                        float px = chunkOriginX + static_cast<float>(x) * TerrainChunk::SAMPLE_SPACING;
                        float dist = std::sqrt((px - worldX) * (px - worldX) + (pz - worldZ) * (pz - worldZ));
                        if (dist <= r) {
                            float f = CalculateFalloff(dist, r, brush.falloff);
                            float currentH = chunk->GetHeight(x, z);
                            float delta = brush.strength * f * 5.0f * (deltaTime / 0.016f);
                            if (brush.invert) delta = -delta;

                            float newH = currentH;
                            switch (brush.tool) {
                                case SculptTool::RAISE:
                                    newH = currentH + delta;
                                    break;
                                case SculptTool::LOWER:
                                    newH = currentH - delta;
                                    break;
                                case SculptTool::FLATTEN: {
                                    float diff = brush.targetHeight - currentH;
                                    newH = currentH + diff * std::min(1.0f, brush.strength * f * 0.5f);
                                    break;
                                }
                                case SculptTool::SET_HEIGHT:
                                    newH = brush.targetHeight;
                                    break;
                                case SculptTool::SMOOTH: {
                                    float avg = 0.0f;
                                    int32_t count = 0;
                                    for (int32_t dz = -1; dz <= 1; ++dz) {
                                        for (int32_t dx = -1; dx <= 1; ++dx) {
                                            int32_t nx = x + dx;
                                            int32_t nz = z + dz;
                                            if (nx >= 0 && nx < TerrainChunk::RESOLUTION && nz >= 0 && nz < TerrainChunk::RESOLUTION) {
                                                avg += chunk->GetHeight(nx, nz);
                                                count++;
                                            }
                                        }
                                    }
                                    if (count > 0) {
                                        avg /= static_cast<float>(count);
                                        newH = currentH + (avg - currentH) * std::min(1.0f, brush.strength * f * 0.5f);
                                    }
                                    break;
                                }
                                default:
                                    newH = currentH + delta;
                                    break;
                            }
                            chunk->SetHeight(x, z, newH);
                        }
                    }
                }
            }
        }
    }
}

std::unique_ptr<TerrainUndoStroke> TerrainWorld::EndStroke() {
    if (!m_inStroke) return nullptr;
    m_inStroke = false;

    auto stroke = std::make_unique<TerrainUndoStroke>();
    for (const auto& [key, initialH] : m_strokeInitialHeights) {
        auto chunk = GetChunk(key.first, key.second);
        if (chunk) {
            ChunkStrokeSnapshot snap;
            snap.chunkX = key.first;
            snap.chunkZ = key.second;
            snap.beforeHeightmap = initialH;
            snap.afterHeightmap = chunk->GetHeightmap();

            auto splatIt = m_strokeInitialSplats.find(key);
            if (splatIt != m_strokeInitialSplats.end()) {
                snap.beforeSplat = splatIt->second;
                snap.afterSplat = chunk->ExportSplatData();
            }

            stroke->chunkSnapshots.push_back(std::move(snap));
        }
    }

    m_strokeInitialHeights.clear();
    m_strokeInitialSplats.clear();

    // Push to undo stack
    m_undoStack.push_back(std::make_unique<TerrainUndoStroke>(*stroke));
    m_redoStack.clear();

    // Cap undo stack to ~64 MB (Doc 10 §A3)
    if (m_undoStack.size() > 50) {
        m_undoStack.erase(m_undoStack.begin());
    }

    return stroke;
}

bool TerrainWorld::Undo() {
    if (m_undoStack.empty()) return false;
    auto stroke = std::move(m_undoStack.back());
    m_undoStack.pop_back();

    stroke->ApplyUndo(m_chunks);
    m_redoStack.push_back(std::move(stroke));
    return true;
}

bool TerrainWorld::Redo() {
    if (m_redoStack.empty()) return false;
    auto stroke = std::move(m_redoStack.back());
    m_redoStack.pop_back();

    stroke->ApplyRedo(m_chunks);
    m_undoStack.push_back(std::move(stroke));
    return true;
}

std::vector<TerrainWorld::SavedChunkFile> TerrainWorld::ExportModifiedChunks() const {
    std::vector<SavedChunkFile> files;
    for (const auto& [key, chunk] : m_chunks) {
        if (chunk && chunk->IsModified()) {
            std::string baseName = "terrain/h_" + std::to_string(key.first) + "_" + std::to_string(key.second) + ".r16";
            files.push_back({baseName, chunk->ExportR16()});

            std::string splatName = "terrain/s_" + std::to_string(key.first) + "_" + std::to_string(key.second) + ".bin";
            files.push_back({splatName, chunk->ExportSplatData()});
        }
    }
    return files;
}

} // namespace qivforge::terrain
