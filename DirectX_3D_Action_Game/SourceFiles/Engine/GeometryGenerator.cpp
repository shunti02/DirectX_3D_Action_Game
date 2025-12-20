/*===================================================================
// ファイル: GeometryGenerator.cpp
// 概要: プリミティブ形状生成ヘルパー（実装部）
=====================================================================*/
#include "Engine/GeometryGenerator.h"
#include <cmath>

using namespace DirectX;

// 統合生成関数
MeshData GeometryGenerator::CreateMesh(ShapeType type, DirectX::XMFLOAT4 color) {
    switch (type) {
    case ShapeType::CUBE:
        return CreateCube(1.0f, color);
    case ShapeType::CONE:
        return CreateCone(0.5f, 1.0f, 20, color);
    case ShapeType::CAPSULE:
        return CreateCapsule(0.5f, 2.0f, 20, color);
    case ShapeType::SPHERE:
        return CreateSphere(0.5f, 16, 16, color);
    case ShapeType::TETRAHEDRON:
        return CreateTetrahedron(1.0f, color);
    default:
        return CreateCube(1.0f, color);
    }
}

// 立方体作成
MeshData GeometryGenerator::CreateCube(float size, DirectX::XMFLOAT4 color) {
    MeshData mesh;
    float h = size * 0.5f;

    mesh.vertices = {
        { -h,  h, -h,   color.x, color.y, color.z, color.w }, // 0
        {  h,  h, -h,   color.x, color.y, color.z, color.w }, // 1
        {  h,  h,  h,   color.x, color.y, color.z, color.w }, // 2
        { -h,  h,  h,   color.x, color.y, color.z, color.w }, // 3
        { -h, -h, -h,   color.x, color.y, color.z, color.w }, // 4
        {  h, -h, -h,   color.x, color.y, color.z, color.w }, // 5
        {  h, -h,  h,   color.x, color.y, color.z, color.w }, // 6
        { -h, -h,  h,   color.x, color.y, color.z, color.w }, // 7
    };

    mesh.indices = {
        0,1,2, 0,2,3, // 上
        4,6,5, 4,7,6, // 下
        4,5,1, 4,1,0, // 奥
        3,2,6, 3,6,7, // 手前
        1,5,6, 1,6,2, // 右
        4,0,3, 4,3,7  // 左
    };
    return mesh;
}

// 円錐作成
MeshData GeometryGenerator::CreateCone(float radius, float height, int sliceCount, DirectX::XMFLOAT4 color) {
    MeshData mesh;

    // 1. 頂点の生成
    // 先っちょ (頂点0)
    mesh.vertices.push_back({ 0.0f, height * 0.5f, 0.0f,  color.x, color.y, color.z, color.w });

    // 底面の円周上の点
    float y = -height * 0.5f;
    float dTheta = XM_2PI / sliceCount;
    for (int i = 0; i <= sliceCount; ++i) {
        float x = radius * cosf(i * dTheta);
        float z = radius * sinf(i * dTheta);
        mesh.vertices.push_back({ x, y, z,  color.x, color.y, color.z, color.w });
    }

    // 底面の中心点
    mesh.vertices.push_back({ 0.0f, y, 0.0f,  color.x, color.y, color.z, color.w });
    int centerIndex = (int)mesh.vertices.size() - 1;

    // 2. インデックス
    // 側面
    for (int i = 0; i < sliceCount; ++i) {
        mesh.indices.push_back(0);
        mesh.indices.push_back(i + 2);
        mesh.indices.push_back(i + 1);
    }
    // 底面
    for (int i = 0; i < sliceCount; ++i) {
        mesh.indices.push_back(centerIndex);
        mesh.indices.push_back(i + 1);
        mesh.indices.push_back(i + 2);
    }
    return mesh;
}

// カプセル作成
MeshData GeometryGenerator::CreateCapsule(float radius, float height, int sliceCount, DirectX::XMFLOAT4 color) {
    MeshData mesh;

    if (height < 2.0f * radius) height = 2.0f * radius;
    float cylinderHeight = height - 2.0f * radius;
    float halfCylinderHeight = cylinderHeight * 0.5f;

    int stackCount = sliceCount / 2;
    if (stackCount < 1) stackCount = 1;

    // 上半球
    for (int i = 0; i <= stackCount; ++i) {
        float phi = XM_PIDIV2 * (float)i / stackCount;
        float y = radius * sinf(phi);
        float r = radius * cosf(phi);
        float yOffset = halfCylinderHeight;

        for (int j = 0; j <= sliceCount; ++j) {
            float theta = XM_2PI * (float)j / sliceCount;
            float x = r * cosf(theta);
            float z = r * sinf(theta);
            mesh.vertices.push_back({ x, y + yOffset, z, color.x, color.y, color.z, color.w });
        }
    }

    // 下半球
    for (int i = 0; i <= stackCount; ++i) {
        float phi = XM_PIDIV2 * (float)i / stackCount;
        float y = -radius * sinf(phi);
        float r = radius * cosf(phi);
        float yOffset = -halfCylinderHeight;

        for (int j = 0; j <= sliceCount; ++j) {
            float theta = XM_2PI * (float)j / sliceCount;
            float x = r * cosf(theta);
            float z = r * sinf(theta);
            mesh.vertices.push_back({ x, y + yOffset, z, color.x, color.y, color.z, color.w });
        }
    }

    // インデックス生成
    int ringVertexCount = sliceCount + 1;

    // [A] 上半球
    for (int i = 0; i < stackCount; ++i) {
        for (int j = 0; j < sliceCount; ++j) {
            int row1 = i * ringVertexCount;
            int row2 = (i + 1) * ringVertexCount;

            mesh.indices.push_back(row1 + j);
            mesh.indices.push_back(row2 + j + 1);
            mesh.indices.push_back(row2 + j);

            mesh.indices.push_back(row1 + j);
            mesh.indices.push_back(row1 + j + 1);
            mesh.indices.push_back(row2 + j + 1);
        }
    }

    // [B] 下半球
    int bottomStart = (stackCount + 1) * ringVertexCount;
    for (int i = 0; i < stackCount; ++i) {
        for (int j = 0; j < sliceCount; ++j) {
            int row1 = bottomStart + i * ringVertexCount;
            int row2 = bottomStart + (i + 1) * ringVertexCount;

            mesh.indices.push_back(row1 + j);
            mesh.indices.push_back(row2 + j);
            mesh.indices.push_back(row2 + j + 1);

            mesh.indices.push_back(row1 + j);
            mesh.indices.push_back(row2 + j + 1);
            mesh.indices.push_back(row1 + j + 1);
        }
    }

    // [C] 胴体
    int topHemisphereBottomRing = 0;
    int bottomHemisphereTopRing = bottomStart;

    for (int j = 0; j < sliceCount; ++j) {
        int top1 = topHemisphereBottomRing + j;
        int top2 = topHemisphereBottomRing + j + 1;
        int bot1 = bottomHemisphereTopRing + j;
        int bot2 = bottomHemisphereTopRing + j + 1;

        mesh.indices.push_back(top1);
        mesh.indices.push_back(bot1);
        mesh.indices.push_back(bot2);

        mesh.indices.push_back(top1);
        mesh.indices.push_back(bot2);
        mesh.indices.push_back(top2);
    }

    return mesh;
}

// ★新規追加: 球体作成
MeshData GeometryGenerator::CreateSphere(float radius, int sliceCount, int stackCount, DirectX::XMFLOAT4 color) {
    MeshData mesh;

    // 1. 頂点生成
    // 北極
    mesh.vertices.push_back({ 0.0f, radius, 0.0f, color.x, color.y, color.z, color.w });

    float phiStep = XM_PI / stackCount;
    float thetaStep = 2.0f * XM_PI / sliceCount;

    for (int i = 1; i <= stackCount - 1; ++i) {
        float phi = i * phiStep;
        for (int j = 0; j <= sliceCount; ++j) {
            float theta = j * thetaStep;

            float x = radius * sinf(phi) * cosf(theta);
            float y = radius * cosf(phi);
            float z = radius * sinf(phi) * sinf(theta);

            mesh.vertices.push_back({ x, y, z, color.x, color.y, color.z, color.w });
        }
    }

    // 南極
    mesh.vertices.push_back({ 0.0f, -radius, 0.0f, color.x, color.y, color.z, color.w });

    // 2. インデックス生成
    // 北極周辺
    for (int i = 1; i <= sliceCount; ++i) {
        mesh.indices.push_back(0);
        mesh.indices.push_back(i + 1);
        mesh.indices.push_back(i);
    }

    // 側面
    int baseIndex = 1;
    int ringVertexCount = sliceCount + 1;
    for (int i = 0; i < stackCount - 2; ++i) {
        for (int j = 0; j < sliceCount; ++j) {
            mesh.indices.push_back(baseIndex + i * ringVertexCount + j);
            mesh.indices.push_back(baseIndex + i * ringVertexCount + j + 1);
            mesh.indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);

            mesh.indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
            mesh.indices.push_back(baseIndex + i * ringVertexCount + j + 1);
            mesh.indices.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
        }
    }

    // 南極周辺
    int southPoleIndex = (int)mesh.vertices.size() - 1;
    baseIndex = southPoleIndex - ringVertexCount;
    for (int i = 0; i < sliceCount; ++i) {
        mesh.indices.push_back(southPoleIndex);
        mesh.indices.push_back(baseIndex + i);
        mesh.indices.push_back(baseIndex + i + 1);
    }

    return mesh;
}
MeshData GeometryGenerator::CreateTetrahedron(float size, DirectX::XMFLOAT4 color) {
    MeshData mesh;
    float s = size; // スケール
    float h = s * 0.816f; // 高さ概算

    // 頂点定義 (正四面体の頂点)
    // 0: 上, 1: 手前, 2: 右奥, 3: 左奥
    std::vector<XMFLOAT3> pos = {
        { 0.0f,   h,    0.0f },       // Top
        { 0.0f,  0.0f, -s * 0.577f }, // Front
        { s * 0.5f, 0.0f,  s * 0.289f }, // Back Right
        {-s * 0.5f, 0.0f,  s * 0.289f }  // Back Left
    };

    // 頂点データ (法線は簡易的に中心から外へ)
    for (const auto& p : pos) {
        XMVECTOR v = XMLoadFloat3(&p);
        XMFLOAT3 n;
        XMStoreFloat3(&n, XMVector3Normalize(v));
        mesh.vertices.push_back({
            p.x, p.y, p.z,          // 位置 (x, y, z)
            color.x, color.y, color.z, color.w // 色 (r, g, b, a)
            });
    }

    // インデックス (4つの面)
    mesh.indices = {
        0, 2, 1, // 右面
        0, 1, 3, // 左面
        0, 3, 2, // 裏面
        1, 2, 3  // 底面
    };

    return mesh;
}