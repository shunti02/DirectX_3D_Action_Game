#pragma once
#include "Vertex.h"
#include <vector>
#include <DirectXMath.h>
#include <cstdint>

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};
enum class ShapeType {
    CUBE,
    CAPSULE,
    SPHERE,
    TETRAHEDRON,
    TORUS,
    DOUBLE_PYRAMID,
    CYLINDER,
    CONE,
    PRISM,
    HEXAGONAL_PRISM,
    TRUNCATED_CONE,
    WEDGE
};

class GeometryGenerator {
public:
    static MeshData CreateMesh(ShapeType type, DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f });

    static MeshData CreateCube(float size, DirectX::XMFLOAT4 color);
    static MeshData CreateCapsule(float radius, float height, int sliceCount, DirectX::XMFLOAT4 color);
    static MeshData CreateSphere(float radius, int sliceCount, int stackCount, DirectX::XMFLOAT4 color);
    static MeshData CreateTetrahedron(float size, DirectX::XMFLOAT4 color);
    static MeshData CreateTorus(float outerRadius, float innerRadius, int radialSegments, int tubularSegments, DirectX::XMFLOAT4 color);
    static MeshData CreateDoublePyramid(float size, float height, DirectX::XMFLOAT4 color);

    static MeshData CreateCylinder(float radius, float height, int sliceCount, DirectX::XMFLOAT4 color);
    static MeshData CreateCone(float radius, float height, int sliceCount, DirectX::XMFLOAT4 color);
    static MeshData CreatePrism(float width, float height, float depth, DirectX::XMFLOAT4 color);
    static MeshData CreateTruncatedCone(float bottomRadius, float topRadius, float height, int sliceCount, DirectX::XMFLOAT4 color);
    static MeshData CreateWedge(float width, float height, float depth, DirectX::XMFLOAT4 color);
};