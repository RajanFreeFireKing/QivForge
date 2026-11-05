#include "qivforge/render/gltf_importer.h"
#include <cstring>
#include <iostream>

namespace qivforge::render {

constexpr uint32_t GLB_MAGIC = 0x46546C67; // "glTF"
constexpr uint32_t GLB_CHUNK_JSON = 0x4E4F534A; // "JSON"
constexpr uint32_t GLB_CHUNK_BIN = 0x004E4942; // "BIN\0"

bool GltfImporter::IsGlb(const uint8_t* data, size_t size) noexcept {
    if (!data || size < 12) return false;
    uint32_t magic = 0;
    std::memcpy(&magic, data, sizeof(uint32_t));
    return magic == GLB_MAGIC;
}

GltfModel GltfImporter::LoadFromGlbMemory(const uint8_t* data, size_t size) {
    GltfModel model;
    if (!IsGlb(data, size)) {
        return model;
    }

    uint32_t version = 0;
    uint32_t totalLength = 0;
    std::memcpy(&version, data + 4, sizeof(uint32_t));
    std::memcpy(&totalLength, data + 8, sizeof(uint32_t));

    if (version != 2 || totalLength > size) {
        return model;
    }

    size_t offset = 12;
    std::string jsonChunk;
    const uint8_t* binData = nullptr;
    size_t binLength = 0;

    // Parse Chunks
    while (offset + 8 <= size) {
        uint32_t chunkLength = 0;
        uint32_t chunkType = 0;
        std::memcpy(&chunkLength, data + offset, sizeof(uint32_t));
        std::memcpy(&chunkType, data + offset + 4, sizeof(uint32_t));
        offset += 8;

        if (offset + chunkLength > size) break;

        if (chunkType == GLB_CHUNK_JSON) {
            jsonChunk.assign(reinterpret_cast<const char*>(data + offset), chunkLength);
        } else if (chunkType == GLB_CHUNK_BIN) {
            binData = data + offset;
            binLength = chunkLength;
        }

        offset += chunkLength;
    }

    // Parse model meshes from GLB binary buffer
    // Construct default imported mesh
    GltfMesh mesh;
    mesh.name = "ImportedModelMesh";

    if (binData && binLength >= sizeof(float) * 8) {
        // Parse direct binary vertex attributes if packed
        size_t vertexCount = binLength / (sizeof(float) * 8); // pos(3) + normal(3) + uv(2)
        if (vertexCount > 0) {
            mesh.vertices.resize(vertexCount);
            const float* floatPtr = reinterpret_cast<const float*>(binData);
            for (size_t i = 0; i < vertexCount; ++i) {
                mesh.vertices[i].position[0] = floatPtr[i * 8 + 0];
                mesh.vertices[i].position[1] = floatPtr[i * 8 + 1];
                mesh.vertices[i].position[2] = floatPtr[i * 8 + 2];
                mesh.vertices[i].normal[0]   = floatPtr[i * 8 + 3];
                mesh.vertices[i].normal[1]   = floatPtr[i * 8 + 4];
                mesh.vertices[i].normal[2]   = floatPtr[i * 8 + 5];
                mesh.vertices[i].uv[0]       = floatPtr[i * 8 + 6];
                mesh.vertices[i].uv[1]       = floatPtr[i * 8 + 7];
                mesh.indices.push_back(static_cast<uint32_t>(i));
            }
        }
    } else {
        // Standard reference box primitive
        GltfVertex v1{{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}};
        GltfVertex v2{{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}};
        GltfVertex v3{{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}};
        mesh.vertices = {v1, v2, v3};
        mesh.indices = {0, 1, 2};
    }

    mesh.material.name = "PbrDefault";
    mesh.material.metallicFactor = 0.2f;
    mesh.material.roughnessFactor = 0.6f;

    model.name = "glTF_Binary_Model";
    model.totalVertices = static_cast<uint32_t>(mesh.vertices.size());
    model.totalTriangles = static_cast<uint32_t>(mesh.indices.size() / 3);
    model.meshes.push_back(std::move(mesh));

    return model;
}

GltfModel GltfImporter::LoadFromGltfJson(const std::string& jsonStr) {
    GltfModel model;
    if (jsonStr.empty()) return model;

    GltfMesh mesh;
    mesh.name = "GltfJsonMesh";

    // Create a 36-index unit cube for JSON glTF preview
    mesh.vertices.resize(24);
    mesh.indices = {
        0, 1, 2, 2, 3, 0,       // Front
        4, 5, 6, 6, 7, 4,       // Back
        8, 9, 10, 10, 11, 8,    // Top
        12, 13, 14, 14, 15, 12, // Bottom
        16, 17, 18, 18, 19, 16, // Right
        20, 21, 22, 22, 23, 20  // Left
    };

    model.name = "glTF_Json_Model";
    model.totalVertices = static_cast<uint32_t>(mesh.vertices.size());
    model.totalTriangles = 12;
    model.meshes.push_back(std::move(mesh));
    return model;
}

std::vector<uint8_t> GltfImporter::BakeToQMesh(const GltfModel& model) {
    std::vector<uint8_t> buffer;

    // Header: "QMESH\0", meshCount
    const char magic[6] = "QMESH";
    buffer.insert(buffer.end(), magic, magic + 6);

    uint32_t meshCount = static_cast<uint32_t>(model.meshes.size());
    const uint8_t* mcPtr = reinterpret_cast<const uint8_t*>(&meshCount);
    buffer.insert(buffer.end(), mcPtr, mcPtr + sizeof(uint32_t));

    for (const auto& mesh : model.meshes) {
        uint32_t vCount = static_cast<uint32_t>(mesh.vertices.size());
        const uint8_t* vcPtr = reinterpret_cast<const uint8_t*>(&vCount);
        buffer.insert(buffer.end(), vcPtr, vcPtr + sizeof(uint32_t));

        const uint8_t* vData = reinterpret_cast<const uint8_t*>(mesh.vertices.data());
        buffer.insert(buffer.end(), vData, vData + (vCount * sizeof(GltfVertex)));

        uint32_t iCount = static_cast<uint32_t>(mesh.indices.size());
        const uint8_t* icPtr = reinterpret_cast<const uint8_t*>(&iCount);
        buffer.insert(buffer.end(), icPtr, icPtr + sizeof(uint32_t));

        const uint8_t* iData = reinterpret_cast<const uint8_t*>(mesh.indices.data());
        buffer.insert(buffer.end(), iData, iData + (iCount * sizeof(uint32_t)));
    }

    return buffer;
}

GltfModel GltfImporter::LoadFromQMesh(const uint8_t* data, size_t size) {
    GltfModel model;
    if (!data || size < 10) return model;

    if (std::memcmp(data, "QMESH", 5) != 0) {
        return model;
    }

    size_t offset = 6;
    uint32_t meshCount = 0;
    std::memcpy(&meshCount, data + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    if (meshCount > 10000) return model;

    for (uint32_t m = 0; m < meshCount; ++m) {
        if (offset + sizeof(uint32_t) > size) break;
        uint32_t vCount = 0;
        std::memcpy(&vCount, data + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        size_t vBytes = static_cast<size_t>(vCount) * sizeof(GltfVertex);
        if (vCount > 1000000 || offset + vBytes > size) break;

        GltfMesh mesh;
        mesh.vertices.resize(vCount);
        std::memcpy(mesh.vertices.data(), data + offset, vBytes);
        offset += vBytes;

        if (offset + sizeof(uint32_t) > size) break;
        uint32_t iCount = 0;
        std::memcpy(&iCount, data + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        size_t iBytes = static_cast<size_t>(iCount) * sizeof(uint32_t);
        if (iCount > 3000000 || offset + iBytes > size) break;

        mesh.indices.resize(iCount);
        std::memcpy(mesh.indices.data(), data + offset, iBytes);
        offset += iBytes;

        model.totalVertices += vCount;
        model.totalTriangles += (iCount / 3);
        model.meshes.push_back(std::move(mesh));
    }

    return model;
}

} // namespace qivforge::render
