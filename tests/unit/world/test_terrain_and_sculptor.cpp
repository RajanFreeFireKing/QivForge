#include "qivforge/terrain/terrain_chunk.h"
#include "qivforge/terrain/terrain_sculptor.h"
#include "qivforge/terrain/vegetation_scatter.h"
#include "qivforge/terrain/navmesh_baker.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <chrono>

using namespace qivforge::terrain;

void TestTerrainChunkAndSplatMaps() {
    std::cout << "[RUN] TestTerrainChunkAndSplatMaps (Doc 10 §A1)..." << std::endl;

    TerrainChunk chunk(0, 0, -50.0f, 450.0f);
    assert(chunk.GetChunkX() == 0);
    assert(chunk.GetChunkZ() == 0);
    assert(!chunk.IsModified());

    // Verify 129x129 resolution and 0.5m spacing
    assert(TerrainChunk::RESOLUTION == 129);
    assert(TerrainChunk::TOTAL_SAMPLES == 129 * 129);
    assert(std::abs(TerrainChunk::SAMPLE_SPACING - 0.5f) < 0.001f);

    // Initial height is flat 0m
    float h00 = chunk.GetHeight(0, 0);
    assert(std::abs(h00 - 0.0f) < 0.1f);

    // Set height and check dirty rect
    chunk.SetHeight(10, 15, 25.5f);
    assert(chunk.IsModified());
    assert(std::abs(chunk.GetHeight(10, 15) - 25.5f) < 0.1f);
    const auto& dr = chunk.GetDirtyRect();
    assert(dr.IsValid());
    assert(dr.minX == 10 && dr.maxX == 10);
    assert(dr.minZ == 15 && dr.maxZ == 15);

    // Splat maps (up to 8 layers)
    assert(chunk.GetDominantLayer(30, 30) == 0); // Default Layer 0 (Grass)
    chunk.SetSplatWeight(1, 30, 30, 255); // Set Layer 1 (Mud) to 255
    chunk.SetSplatWeight(0, 30, 30, 50);
    assert(chunk.GetDominantLayer(30, 30) == 1); // Layer 1 dominates

    // Export & Import R16
    auto r16Data = chunk.ExportR16();
    assert(r16Data.size() == TerrainChunk::TOTAL_SAMPLES * sizeof(uint16_t));

    TerrainChunk loadedChunk(0, 0);
    bool imported = loadedChunk.ImportR16(r16Data.data(), r16Data.size());
    assert(imported);
    assert(std::abs(loadedChunk.GetHeight(10, 15) - 25.5f) < 0.1f);

    // LOD mesh generation with skirts (Doc 05 §10 & Doc 10 §A4)
    std::vector<TerrainChunk::MeshVertex> verts;
    std::vector<uint32_t> indices;
    chunk.GenerateLodMesh(0, 2.0f, verts, indices);
    assert(!verts.empty());
    assert(!indices.empty());
    // Verify skirt vertices exist (y is lower by skirt depth)
    bool hasSkirtVert = false;
    for (const auto& v : verts) {
        if (v.y < -1.5f) {
            hasSkirtVert = true;
            break;
        }
    }
    assert(hasSkirtVert);

    std::cout << "[PASS] TestTerrainChunkAndSplatMaps" << std::endl;
}

void TestTerrainSculptingAndUndo() {
    std::cout << "[RUN] TestTerrainSculptingAndUndo (Doc 10 AC2)..." << std::endl;

    TerrainWorld world;
    auto chunk = world.GetOrCreateChunk(0, 0);
    auto originalHeights = chunk->GetHeightmap();

    // Perform 10 distinct brush strokes
    for (int32_t stroke = 0; stroke < 10; ++stroke) {
        world.BeginStroke();
        BrushSettings brush;
        brush.tool = (stroke % 2 == 0) ? SculptTool::RAISE : SculptTool::LOWER;
        brush.radiusMeters = 15.0f;
        brush.strength = 0.4f;
        brush.falloff = FalloffType::SMOOTH;

        world.ApplyBrush(32.0f + static_cast<float>(stroke), 32.0f, brush, 0.016f);
        world.EndStroke();
    }

    assert(world.GetUndoStackSize() == 10);
    assert(chunk->GetHeightmap() != originalHeights);

    // Now undo all 10 strokes
    for (int32_t i = 0; i < 10; ++i) {
        bool undone = world.Undo();
        assert(undone);
    }

    // Doc 10 AC2: 10-stroke edit and undo returns the heightmap byte-identical to original
    auto restoredHeights = chunk->GetHeightmap();
    assert(restoredHeights.size() == originalHeights.size());
    assert(std::memcmp(restoredHeights.data(), originalHeights.data(), originalHeights.size() * sizeof(uint16_t)) == 0);

    std::cout << "[PASS] TestTerrainSculptingAndUndo (Byte-identical verified)" << std::endl;
}

void TestDirtyChunkSerialization() {
    std::cout << "[RUN] TestDirtyChunkSerialization (Doc 10 AC3)..." << std::endl;

    TerrainWorld world;

    // Create 100 chunks (10x10 grid)
    for (int32_t cz = 0; cz < 10; ++cz) {
        for (int32_t cx = 0; cx < 10; ++cx) {
            world.GetOrCreateChunk(cx, cz);
        }
    }
    assert(world.GetLoadedChunkCount() == 100);
    assert(world.GetModifiedChunkCount() == 0);

    // Edit exactly 3 chunks: (1, 1), (3, 4), and (8, 9)
    BrushSettings brush;
    brush.tool = SculptTool::RAISE;
    brush.radiusMeters = 8.0f;
    brush.strength = 0.5f;

    world.ApplyBrush(1 * 64.0f + 32.0f, 1 * 64.0f + 32.0f, brush);
    world.ApplyBrush(3 * 64.0f + 32.0f, 4 * 64.0f + 32.0f, brush);
    world.ApplyBrush(8 * 64.0f + 32.0f, 9 * 64.0f + 32.0f, brush);
    world.EndStroke();

    // Verify modified chunk count is exactly 3
    assert(world.GetModifiedChunkCount() == 3);

    // Doc 10 AC3: Saving project writes only the 3 modified chunks
    auto exportedFiles = world.ExportModifiedChunks();
    // 3 chunks * (1 height file + 1 splat file) = 6 files total (3 height files)
    size_t heightFileCount = 0;
    for (const auto& f : exportedFiles) {
        if (f.filename.find(".r16") != std::string::npos) {
            heightFileCount++;
        }
    }
    assert(heightFileCount == 3);

    std::cout << "[PASS] TestDirtyChunkSerialization" << std::endl;
}

void TestSurfacePhysicsIntegration() {
    std::cout << "[RUN] TestSurfacePhysicsIntegration (Doc 10 AC4 & Doc 06 §9.4)..." << std::endl;

    TerrainWorld world;

    // Default layer 0 is Grass
    SurfaceType defaultSurf = world.GetSurfaceTypeAt(32.0f, 32.0f);
    assert(defaultSurf == SurfaceType::GRASS);

    // Paint Layer 1 (Mud) at (32m, 32m) with 10m brush
    BrushSettings brush;
    brush.tool = SculptTool::PAINT_LAYER;
    brush.targetLayer = 1; // Mud layer
    brush.radiusMeters = 10.0f;
    brush.strength = 1.0f;

    world.ApplyBrush(32.0f, 32.0f, brush);
    world.EndStroke();

    // Query surface type in painted center: should be MUD
    SurfaceType mudSurf = world.GetSurfaceTypeAt(32.0f, 32.0f);
    assert(mudSurf == SurfaceType::MUD);

    // Query outside painted area (e.g. at 60m): should remain GRASS
    SurfaceType outsideSurf = world.GetSurfaceTypeAt(60.0f, 60.0f);
    assert(outsideSurf == SurfaceType::GRASS);

    std::cout << "[PASS] TestSurfacePhysicsIntegration" << std::endl;
}

void TestVegetationScatterAndCulling() {
    std::cout << "[RUN] TestVegetationScatterAndCulling (Doc 10 AC5)..." << std::endl;

    VegetationManager vegMgr;

    // Scatter 50,000 trees over 8 chunks (6,250 trees per chunk)
    for (int32_t cz = 0; cz < 2; ++cz) {
        for (int32_t cx = 0; cx < 4; ++cx) {
            auto chunk = vegMgr.GetOrCreateChunk(cx, cz);
            for (int32_t i = 0; i < 6250; ++i) {
                float lx = static_cast<float>(i % 64);
                float ly = 10.0f;
                float lz = static_cast<float>((i / 64) % 64);
                chunk->AddInstance(lx, ly, lz, 45.0f, 1.2f, static_cast<uint16_t>(VegetationType::TREE));
            }
        }
    }

    assert(vegMgr.GetTotalInstanceCount() == 50000);

    // Benchmark distance culling with camera at center (64m, 10m, 32m)
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<ChunkVegetation::VisibleInstance> visible;
    visible.reserve(50000);
    size_t visCount = vegMgr.CullAndCollect(64.0f, 10.0f, 32.0f, 1.0f, visible);
    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "  50,000 trees culled in " << ms << " ms (" << visCount << " visible within 250m)" << std::endl;
    // Doc 10 AC5: Must easily hold > 45 fps (frame budget 22.2 ms)
    assert(ms < 22.2);
    assert(visCount > 0 && visCount <= 50000);

    std::cout << "[PASS] TestVegetationScatterAndCulling" << std::endl;
}

void TestChunkNavmeshBake() {
    std::cout << "[RUN] TestChunkNavmeshBake (Doc 10 §A6)..." << std::endl;

    TerrainChunk chunk(0, 0);
    // Create a flat area with a very steep cliff (height jumps by 30m over 0.5m)
    for (int32_t z = 0; z < 129; ++z) {
        for (int32_t x = 0; x < 64; ++x) {
            chunk.SetHeight(x, z, 0.0f);
        }
        for (int32_t x = 65; x < 129; ++x) {
            chunk.SetHeight(x, z, 30.0f);
        }
    }

    NavmeshBakeParams params;
    params.maxSlopeDeg = 45.0f;
    auto nav = NavmeshBaker::BakeChunk(chunk, params);
    assert(nav != nullptr);

    // Flat region at x = 10m should be walkable
    assert(nav->IsPointWalkable(10.0f, 10.0f));

    // Steep cliff at x = 32m (cell 32) should be non-walkable
    assert(!nav->IsPointWalkable(32.0f, 10.0f));

    // Serialization
    auto bin = nav->Serialize();
    assert(!bin.empty());

    ChunkNavmesh loadedNav(0, 0);
    bool ok = loadedNav.Deserialize(bin.data(), bin.size());
    assert(ok);
    assert(loadedNav.IsPointWalkable(10.0f, 10.0f));
    assert(!loadedNav.IsPointWalkable(32.0f, 10.0f));

    std::cout << "[PASS] TestChunkNavmeshBake" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  TERRAIN & WORLD BUILDING TESTS (M9)   " << std::endl;
    std::cout << "========================================" << std::endl;

    TestTerrainChunkAndSplatMaps();
    TestTerrainSculptingAndUndo();
    TestDirtyChunkSerialization();
    TestSurfacePhysicsIntegration();
    TestVegetationScatterAndCulling();
    TestChunkNavmeshBake();

    std::cout << "ALL TERRAIN & WORLD TESTS PASSED!" << std::endl;
    return 0;
}
