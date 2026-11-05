#pragma once

#include "qivforge/terrain/terrain_chunk.h"
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <functional>

namespace qivforge::terrain {

enum class SculptTool : uint8_t {
    RAISE = 0,
    LOWER = 1,
    SMOOTH = 2,
    FLATTEN = 3,
    SET_HEIGHT = 4,
    NOISE = 5,
    EROSION = 6,
    PAINT_LAYER = 7,
    FILL_LAYER = 8
};

enum class FalloffType : uint8_t {
    SMOOTH = 0,
    LINEAR = 1,
    SHARP = 2
};

struct BrushSettings {
    SculptTool tool = SculptTool::RAISE;
    FalloffType falloff = FalloffType::SMOOTH;
    float radiusMeters = 10.0f;
    float strength = 0.5f;
    float targetHeight = 0.0f;
    int32_t targetLayer = 0;
    bool invert = false;
};

// Snapshot for undoing a single stroke across affected chunks
struct ChunkStrokeSnapshot {
    int32_t chunkX = 0;
    int32_t chunkZ = 0;
    std::vector<uint16_t> beforeHeightmap;
    std::vector<uint16_t> afterHeightmap;
    std::vector<uint8_t> beforeSplat;
    std::vector<uint8_t> afterSplat;
};

class TerrainUndoStroke {
public:
    std::vector<ChunkStrokeSnapshot> chunkSnapshots;

    void ApplyUndo(std::map<std::pair<int32_t, int32_t>, std::shared_ptr<TerrainChunk>>& chunks);
    void ApplyRedo(std::map<std::pair<int32_t, int32_t>, std::shared_ptr<TerrainChunk>>& chunks);
};

class TerrainWorld {
public:
    TerrainWorld();

    // Chunk management
    std::shared_ptr<TerrainChunk> GetOrCreateChunk(int32_t cx, int32_t cz);
    std::shared_ptr<TerrainChunk> GetChunk(int32_t cx, int32_t cz) const;
    [[nodiscard]] size_t GetLoadedChunkCount() const noexcept { return m_chunks.size(); }
    [[nodiscard]] size_t GetModifiedChunkCount() const noexcept;

    // Layer management (up to 8 layers per terrain)
    void SetLayer(int32_t index, const TerrainLayer& layer);
    [[nodiscard]] const TerrainLayer& GetLayer(int32_t index) const;

    // World queries
    [[nodiscard]] float GetHeightAt(float worldX, float worldZ) const;
    [[nodiscard]] SurfaceType GetSurfaceTypeAt(float worldX, float worldZ) const;

    // Sculpting & Painting API
    // Begins a stroke, returning a stroke undo recording handle
    void BeginStroke();
    void ApplyBrush(float worldX, float worldZ, const BrushSettings& brush, float deltaTime = 0.016f);
    std::unique_ptr<TerrainUndoStroke> EndStroke();

    // Undo / Redo stack for terrain (max 64 MB budget per Doc 10 §A3)
    bool Undo();
    bool Redo();
    [[nodiscard]] size_t GetUndoStackSize() const noexcept { return m_undoStack.size(); }

    // Serialization: writes only modified chunks (Doc 10 AC3)
    struct SavedChunkFile {
        std::string filename;
        std::vector<uint8_t> bytes;
    };
    std::vector<SavedChunkFile> ExportModifiedChunks() const;

private:
    std::map<std::pair<int32_t, int32_t>, std::shared_ptr<TerrainChunk>> m_chunks;
    std::array<TerrainLayer, TerrainChunk::MAX_LAYERS> m_layers;

    bool m_inStroke = false;
    std::map<std::pair<int32_t, int32_t>, std::vector<uint16_t>> m_strokeInitialHeights;
    std::map<std::pair<int32_t, int32_t>, std::vector<uint8_t>> m_strokeInitialSplats;

    std::vector<std::unique_ptr<TerrainUndoStroke>> m_undoStack;
    std::vector<std::unique_ptr<TerrainUndoStroke>> m_redoStack;

    [[nodiscard]] float CalculateFalloff(float dist, float radius, FalloffType type) const noexcept;
};

} // namespace qivforge::terrain
