#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace qivforge::render {

struct GltfVertex {
    float position[3] = {0.0f, 0.0f, 0.0f};
    float normal[3] = {0.0f, 1.0f, 0.0f};
    float uv[2] = {0.0f, 0.0f};
};

struct GltfMaterial {
    std::string name;
    float baseColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float metallicFactor = 0.0f;
    float roughnessFactor = 0.5f;
};

struct GltfMesh {
    std::string name;
    std::vector<GltfVertex> vertices;
    std::vector<uint32_t> indices;
    GltfMaterial material;
};

struct GltfModel {
    std::string name;
    std::vector<GltfMesh> meshes;
    uint32_t totalVertices = 0;
    uint32_t totalTriangles = 0;
};

class GltfImporter {
public:
    static bool IsGlb(const uint8_t* data, size_t size) noexcept;
    static GltfModel LoadFromGlbMemory(const uint8_t* data, size_t size);
    static GltfModel LoadFromGltfJson(const std::string& jsonStr);

    // Baker: serializes to binary .qmesh cache format (Doc 13 §3)
    static std::vector<uint8_t> BakeToQMesh(const GltfModel& model);
    static GltfModel LoadFromQMesh(const uint8_t* data, size_t size);
};

} // namespace qivforge::render
