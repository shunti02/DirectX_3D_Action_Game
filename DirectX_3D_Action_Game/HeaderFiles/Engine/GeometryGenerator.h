/*===================================================================
// ファイル: GeometryGenerator.h
// 概要: プリミティブ形状生成ヘルパー（宣言部）
=====================================================================*/
#pragma once
#include "Vertex.h"
#include <vector>
#include <DirectXMath.h>
#include <cstdint>

// メッシュデータ構造体
struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

// 形状の種類
enum class ShapeType {
    CUBE,
    CONE,
    CAPSULE,
    SPHERE,
    TETRAHEDRON,
    PLANE
};

class GeometryGenerator {
public:
    // 統合生成関数
    static MeshData CreateMesh(ShapeType type, DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f });

    // 個別の生成関数
    static MeshData CreateCube(float size, DirectX::XMFLOAT4 color);
    static MeshData CreateCone(float radius, float height, int sliceCount, DirectX::XMFLOAT4 color);
    static MeshData CreateCapsule(float radius, float height, int sliceCount, DirectX::XMFLOAT4 color);
    static MeshData CreateSphere(float radius, int sliceCount, int stackCount, DirectX::XMFLOAT4 color);
    static MeshData CreateTetrahedron(float size, DirectX::XMFLOAT4 color);
};