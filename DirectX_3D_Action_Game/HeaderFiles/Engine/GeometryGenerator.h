/*===================================================================
//ファイル:GeometryGenerator.h
//概要:プリミティブ形状（立方体、円錐など）の頂点データを作成するヘルパー
//修正: DirectX::XMFLOAT4への型修正、タイプミスの除去、変数スコープの修正
=====================================================================*/
#pragma once
#include "Vertex.h"
#include <vector>
#include <cmath>
#include <DirectXMath.h>

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<UINT> indices;
};

enum class ShapeType {
    CUBE,
    CONE,
    CAPSULE,
    SPHERE
};

class GeometryGenerator {
public:
    // 形状生成の統合関数
    // 色指定引数を追加 (デフォルトは白)
    static MeshData CreateMesh(ShapeType type, DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f }) {
        switch (type) {
        case ShapeType::CUBE:
            return CreateCube(1.0f, color);
        case ShapeType::CONE:
            return CreateCone(0.5f, 1.0f, 20, color);
        case ShapeType::CAPSULE:
            return CreateCapsule(0.5f, 2.0f, 20, color);
        default:
            return CreateCube(1.0f, color);
        }
    }

    // 立方体作成
    static MeshData CreateCube(float size, DirectX::XMFLOAT4 color) {
        MeshData mesh;
        float h = size * 0.5f;

        // 全頂点に指定された color を適用
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

    // カプセル作成
    // 引数の型を DirectX::XMFLOAT4 に修正
    static MeshData CreateCapsule(float radius, float height, int sliceCount, DirectX::XMFLOAT4 color) {
        MeshData mesh;
        if (height < 2.0f * radius) height = 2.0f * radius;
        float cylinderHeight = height - 2.0f * radius;

        // 簡易実装版ロジック
        int sphereRings = sliceCount / 2;
        int totalLat = sphereRings * 2 + 1;

        for (int i = 0; i <= totalLat; ++i) {
            float v = (float)i / totalLat;
            float phi = v * DirectX::XM_PI;

            float y = -radius * cosf(phi);
            if (i > sphereRings) y += cylinderHeight * 0.5f;
            else                 y -= cylinderHeight * 0.5f;

            float r = radius * sinf(phi);

            for (int j = 0; j <= sliceCount; ++j) {
                float u = (float)j / sliceCount;
                float theta = u * DirectX::XM_2PI;

                float x = r * cosf(theta);
                float z = r * sinf(theta);

                // 指定色を適用
                mesh.vertices.push_back({ x, y, z, color.x, color.y, color.z, color.w });
            }
        }

        // インデックス生成
        for (int i = 0; i < totalLat; ++i) {
            for (int j = 0; j < sliceCount; ++j) {
                int nextI = i + 1;
                // sliceCountで割った余りを使ってリングを閉じる
                // (sliceCount + 1) 個の頂点を作っているため、j+1が最後の点なら0に戻すわけではないが
                // ここでは単純に次の列を参照する簡易的な実装
                int p0 = i * (sliceCount + 1) + j;
                int p1 = i * (sliceCount + 1) + (j + 1);
                int p2 = nextI * (sliceCount + 1) + j;
                int p3 = nextI * (sliceCount + 1) + (j + 1);

                mesh.indices.push_back(p0);
                mesh.indices.push_back(p2);
                mesh.indices.push_back(p1);

                mesh.indices.push_back(p1);
                mesh.indices.push_back(p2);
                mesh.indices.push_back(p3);
            }
        }
        return mesh;
    }

    // 円錐作成
    static MeshData CreateCone(float radius, float height, int sliceCount, DirectX::XMFLOAT4 color) {
        MeshData mesh;

        // 1. 頂点の生成
        // 先っちょ (頂点0)
        mesh.vertices.push_back({ 0.0f, height * 0.5f, 0.0f,  color.x, color.y, color.z, color.w });

        // 底面の円周上の点
        float y = -height * 0.5f;
        float dTheta = DirectX::XM_2PI / sliceCount;
        for (int i = 0; i <= sliceCount; ++i) {
            float x = radius * cosf(i * dTheta);
            float z = radius * sinf(i * dTheta);

            // 指定色を適用
            mesh.vertices.push_back({ x, y, z,  color.x, color.y, color.z, color.w });
        }

        // 底面の中心点 (修正: ループ外のため x, z は使わず 0.0f を指定)
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
};