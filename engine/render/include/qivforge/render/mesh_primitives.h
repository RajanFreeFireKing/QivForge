#pragma once

#include <vector>
#include <cstdint>

namespace qivforge::render {

struct Vertex {
    float position[3];
    float normal[3];
    float uv[2];
    float tangent[4];
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
};

class MeshPrimitives {
public:
    /**
     * Generates a 3D procedural cube mesh (24 vertices, 36 indices) with normals, UVs and tangents.
     */
    static MeshData CreateCube(float halfExtent = 0.5f);

    /**
     * Generates a 3D flat plane mesh for ground/terrain foundation.
     */
    static MeshData CreatePlane(float halfWidth = 5.0f, float halfDepth = 5.0f);
};

} // namespace qivforge::render
