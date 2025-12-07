/*===================================================================
//ファイル:GeometryGenerator.h
//概要:プリミティブ形状（立方体、円錐など）の頂点データを作成するヘルパー
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
    static MeshData CreateMesh(ShapeType type) {
        switch (type) {
        case ShapeType::CUBE:
            return CreateCube(1.0f);
        case ShapeType::CONE:
            return CreateCone(0.5f, 1.0f, 20);
        case ShapeType::CAPSULE:
            return CreateCapsule(0.5f, 2.0f, 20);
            // 必要に応じて SPHERE などを追加
        default:
            return CreateCube(1.0f);
        }
    }
    // 立方体作成
    static MeshData CreateCube(float size = 1.0f) {
        MeshData mesh;
        float h = size * 0.5f;

        mesh.vertices = {
            { -h,  h, -h,   1.0f, 0.0f, 0.0f, 1.0f }, // 0: 赤
            {  h,  h, -h,   0.0f, 1.0f, 0.0f, 1.0f }, // 1: 緑
            {  h,  h,  h,   0.0f, 0.0f, 1.0f, 1.0f }, // 2: 青
            { -h,  h,  h,   1.0f, 1.0f, 0.0f, 1.0f }, // 3: 黄
            { -h, -h, -h,   0.0f, 1.0f, 1.0f, 1.0f }, // 4: 水
            {  h, -h, -h,   1.0f, 0.0f, 1.0f, 1.0f }, // 5: 紫
            {  h, -h,  h,   1.0f, 1.0f, 1.0f, 1.0f }, // 6: 白
            { -h, -h,  h,   0.0f, 0.0f, 0.0f, 1.0f }, // 7: 黒
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
    // radius: 半径, height: 全体の高さ (半球含む), sliceCount: 円周の分割数
    static MeshData CreateCapsule(float radius = 0.5f, float height = 2.0f, int sliceCount = 20) {
        MeshData mesh;

        // 高さの制限（球より小さくならないように）
        if (height < 2.0f * radius) height = 2.0f * radius;

        float cylinderHeight = height - 2.0f * radius;
        float stackHeight = cylinderHeight; // 円柱部分の高さ
        int stacks = 1; // 円柱の分割数（縦）

        // 1. 頂点の生成
        // 上の半球
        int rings = sliceCount / 2;
        for (int i = 0; i <= rings; ++i) {
            float phi = i * DirectX::XM_PIDIV2 / rings;
            float y = radius * cosf(phi) + cylinderHeight * 0.5f;
            float r = radius * sinf(phi);
            for (int j = 0; j <= sliceCount; ++j) {
                float theta = j * DirectX::XM_2PI / sliceCount;
                float x = r * cosf(theta);
                float z = r * sinf(theta);
                // 色: 白～赤のグラデーション
                mesh.vertices.push_back({ x, y, z,  1.0f, (float)i / rings, (float)i / rings, 1.0f });
            }
        }

        // 円柱部分
        for (int i = 0; i <= stacks; ++i) {
            float y = -cylinderHeight * 0.5f + (float)i / stacks * cylinderHeight;
            for (int j = 0; j <= sliceCount; ++j) {
                float theta = j * DirectX::XM_2PI / sliceCount;
                float x = radius * cosf(theta);
                float z = radius * sinf(theta);
                // 色: 青っぽい色
                mesh.vertices.push_back({ x, y, z,  0.2f, 0.2f, 1.0f, 1.0f });
            }
        }

        // 下の半球
        for (int i = 0; i <= rings; ++i) {
            float phi = DirectX::XM_PIDIV2 + i * DirectX::XM_PIDIV2 / rings;
            float y = radius * cosf(phi) - cylinderHeight * 0.5f;
            float r = radius * sinf(phi);
            for (int j = 0; j <= sliceCount; ++j) {
                float theta = j * DirectX::XM_2PI / sliceCount;
                float x = r * cosf(theta);
                float z = r * sinf(theta);
                // 色: 白～緑
                mesh.vertices.push_back({ x, y, z,  (float)i / rings, 1.0f, (float)i / rings, 1.0f });
            }
        }

        // 2. インデックスの生成
        int ringVertexCount = sliceCount + 1;
        int totalRings = rings + 1 + stacks + 1 + rings + 1; // 上半球 + 円柱 + 下半球

        // 頂点の並びは上から下へ順番に格納されているため、隣り合うリングを繋ぐ
        // ただし、上記コードではパーツごとにループしているため、実際は頂点配列のオフセット管理が必要
        // 簡易実装として、頂点生成と同時にインデックスを貼るのが難しい構造にしてしまったため
        // 今回は「頂点を全部リストに入れてから、リング状に繋ぐ」方式で一括処理します。

        // ※修正: 上記のループ構造だとインデックス貼りが複雑になるため、
        // もっとシンプルな「円柱 + 上下半球」の結合済み頂点リストとして扱います。

        // --- 簡易版：全頂点を上から下へリング状に繋ぐ ---
        int totalStacks = rings + stacks + rings; // 縦方向の分割総数
        for (int i = 0; i < totalStacks + 2; ++i) { // +2は接合部の重複分などを考慮した大まかな数
            // 今回はロジックが複雑になるため、もっと単純な「球を引き伸ばした形状」として再実装します
        }

        // ★仕切り直し: もっと簡単なロジック（球を引き伸ばす）で再定義します
        mesh.vertices.clear();
        mesh.indices.clear();

        // 緯度方向(phi)の分割数: 半球(rings) + 円柱(1) + 半球(rings)
        int sphereRings = sliceCount / 2;
        int totalLat = sphereRings * 2 + 1;

        for (int i = 0; i <= totalLat; ++i) {
            float v = (float)i / totalLat;
            float phi = v * DirectX::XM_PI;

            // Y座標の計算（ここで引き伸ばす）
            float y = -radius * cosf(phi);

            // 円柱部分の挿入
            if (i > sphereRings) y += cylinderHeight * 0.5f; // 上半球
            else                 y -= cylinderHeight * 0.5f; // 下半球

            // ※真ん中の接合部で頂点を重複させる必要がありますが、今回は簡易的にスムーズに繋ぎます

            float r = radius * sinf(phi);

            for (int j = 0; j <= sliceCount; ++j) {
                float u = (float)j / sliceCount;
                float theta = u * DirectX::XM_2PI;
                float x = r * cosf(theta);
                float z = r * sinf(theta);

                mesh.vertices.push_back({ x, y, z, x + 0.5f, y + 0.5f, z + 0.5f, 1.0f }); // 色は座標ベース
            }
        }

        // インデックス生成
        for (int i = 0; i < totalLat; ++i) {
            for (int j = 0; j < sliceCount; ++j) {
                int nextI = i + 1;
                int nextJ = (j + 1) % (sliceCount + 1); // テクスチャ考慮なら%しないが今回は頂点共有

                // 四角形を2つの三角形に
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

    // 円錐（三角錐含む）作成
    // sliceCount: 円を何分割するか（3なら三角錐、20なら綺麗な円錐）
    static MeshData CreateCone(float radius = 0.5f, float height = 1.0f, int sliceCount = 20) {
        MeshData mesh;

        // 1. 頂点の生成
        // 先っちょ (頂点0)
        mesh.vertices.push_back({ 0.0f, height * 0.5f, 0.0f,  1.0f, 1.0f, 1.0f, 1.0f });

        // 底面の円周上の点
        float y = -height * 0.5f;
        float dTheta = DirectX::XM_2PI / sliceCount;
        for (int i = 0; i <= sliceCount; ++i) { // テクスチャの継ぎ目のため一周分重複させる
            float x = radius * cosf(i * dTheta);
            float z = radius * sinf(i * dTheta);
            // 色は適当にグラデーションさせる
            float c = (float)i / sliceCount;
            mesh.vertices.push_back({ x, y, z,  c, 1.0f - c, 0.5f, 1.0f });
        }
        // 底面の中心点
        mesh.vertices.push_back({ 0.0f, y, 0.0f,  0.5f, 0.5f, 0.5f, 1.0f });
        int centerIndex = (int)mesh.vertices.size() - 1;

        // 2. インデックスの生成
        // 側面（先っちょと底面の点をつなぐ三角形）
        for (int i = 0; i < sliceCount; ++i) {
            mesh.indices.push_back(0);
            mesh.indices.push_back(i + 2); // 次の点
            mesh.indices.push_back(i + 1); // 現在の点
        }
        // 底面（中心点と底面の点をつなぐ三角形）
        for (int i = 0; i < sliceCount; ++i) {
            mesh.indices.push_back(centerIndex);
            mesh.indices.push_back(i + 1);
            mesh.indices.push_back(i + 2);
        }

        return mesh;
    }
};
