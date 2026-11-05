#include "qivforge/render/mesh_primitives.h"

namespace qivforge::render {

MeshData MeshPrimitives::CreateCube(float h) {
    MeshData data;
    data.vertices.reserve(24);
    data.indices.reserve(36);

    // Helper lambda to append a face (4 vertices + 6 indices)
    auto addFace = [&](
        float p0[3], float p1[3], float p2[3], float p3[3],
        float n[3], float t[4]
    ) {
        uint16_t baseIndex = static_cast<uint16_t>(data.vertices.size());

        Vertex v0 = {{p0[0], p0[1], p0[2]}, {n[0], n[1], n[2]}, {0.0f, 0.0f}, {t[0], t[1], t[2], t[3]}};
        Vertex v1 = {{p1[0], p1[1], p1[2]}, {n[0], n[1], n[2]}, {1.0f, 0.0f}, {t[0], t[1], t[2], t[3]}};
        Vertex v2 = {{p2[0], p2[1], p2[2]}, {n[0], n[1], n[2]}, {1.0f, 1.0f}, {t[0], t[1], t[2], t[3]}};
        Vertex v3 = {{p3[0], p3[1], p3[2]}, {n[0], n[1], n[2]}, {0.0f, 1.0f}, {t[0], t[1], t[2], t[3]}};

        data.vertices.push_back(v0);
        data.vertices.push_back(v1);
        data.vertices.push_back(v2);
        data.vertices.push_back(v3);

        // Two triangles: (0, 1, 2) and (0, 2, 3)
        data.indices.push_back(baseIndex + 0);
        data.indices.push_back(baseIndex + 1);
        data.indices.push_back(baseIndex + 2);

        data.indices.push_back(baseIndex + 0);
        data.indices.push_back(baseIndex + 2);
        data.indices.push_back(baseIndex + 3);
    };

    float normXPos[3] = {1.0f, 0.0f, 0.0f};
    float normXNeg[3] = {-1.0f, 0.0f, 0.0f};
    float normYPos[3] = {0.0f, 1.0f, 0.0f};
    float normYNeg[3] = {0.0f, -1.0f, 0.0f};
    float normZPos[3] = {0.0f, 0.0f, 1.0f};
    float normZNeg[3] = {0.0f, 0.0f, -1.0f};

    float tanZ[4] = {1.0f, 0.0f, 0.0f, 1.0f};

    // +Z Face (Front)
    float f0[3] = {-h, -h,  h}, f1[3] = { h, -h,  h}, f2[3] = { h,  h,  h}, f3[3] = {-h,  h,  h};
    addFace(f0, f1, f2, f3, normZPos, tanZ);

    // -Z Face (Back)
    float b0[3] = { h, -h, -h}, b1[3] = {-h, -h, -h}, b2[3] = {-h,  h, -h}, b3[3] = { h,  h, -h};
    addFace(b0, b1, b2, b3, normZNeg, tanZ);

    // +X Face (Right)
    float r0[3] = { h, -h,  h}, r1[3] = { h, -h, -h}, r2[3] = { h,  h, -h}, r3[3] = { h,  h,  h};
    addFace(r0, r1, r2, r3, normXPos, tanZ);

    // -X Face (Left)
    float l0[3] = {-h, -h, -h}, l1[3] = {-h, -h,  h}, l2[3] = {-h,  h,  h}, l3[3] = {-h,  h, -h};
    addFace(l0, l1, l2, l3, normXNeg, tanZ);

    // +Y Face (Top)
    float t0[3] = {-h,  h,  h}, t1[3] = { h,  h,  h}, t2[3] = { h,  h, -h}, t3[3] = {-h,  h, -h};
    addFace(t0, t1, t2, t3, normYPos, tanZ);

    // -Y Face (Bottom)
    float d0[3] = {-h, -h, -h}, d1[3] = { h, -h, -h}, d2[3] = { h, -h,  h}, d3[3] = {-h, -h,  h};
    addFace(d0, d1, d2, d3, normYNeg, tanZ);

    return data;
}

MeshData MeshPrimitives::CreatePlane(float hw, float hd) {
    MeshData data;
    data.vertices.reserve(4);
    data.indices.reserve(6);

    float norm[3] = {0.0f, 1.0f, 0.0f};
    float tan[4] = {1.0f, 0.0f, 0.0f, 1.0f};

    data.vertices.push_back({{-hw, 0.0f,  hd}, {norm[0], norm[1], norm[2]}, {0.0f, 0.0f}, {tan[0], tan[1], tan[2], tan[3]}});
    data.vertices.push_back({{ hw, 0.0f,  hd}, {norm[0], norm[1], norm[2]}, {1.0f, 0.0f}, {tan[0], tan[1], tan[2], tan[3]}});
    data.vertices.push_back({{ hw, 0.0f, -hd}, {norm[0], norm[1], norm[2]}, {1.0f, 1.0f}, {tan[0], tan[1], tan[2], tan[3]}});
    data.vertices.push_back({{-hw, 0.0f, -hd}, {norm[0], norm[1], norm[2]}, {0.0f, 1.0f}, {tan[0], tan[1], tan[2], tan[3]}});

    data.indices.push_back(0);
    data.indices.push_back(1);
    data.indices.push_back(2);
    data.indices.push_back(0);
    data.indices.push_back(2);
    data.indices.push_back(3);

    return data;
}

} // namespace qivforge::render
