/*===================================================================
//ファイル:EntityFactory.h
//概要:Entityのプレハブ（ひな形）作成関数群
//修正: CreateShapeの重複を削除し、Colors.hを使用するように統一
=====================================================================*/
#pragma once
#include "ECS/World.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/CameraComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/ColliderComponent.h"
#include "App/Game.h"
#include "Engine/GeometryGenerator.h"
#include "Engine/Vertex.h"
#include "Engine/Colors.h" // ★色定義のインクルード
#include <vector>
#include <DirectXMath.h>

namespace EntityFactory {
    // カメラを作成する
    inline EntityID CreateCamera(World* world, float x, float y, float z) {
        return world->CreateEntity()
            .AddComponent<TransformComponent>(x, y, z)
            .AddComponent<CameraComponent>()
            .Build();
    }

    // 汎用メッシュ作成ヘルパー
    inline void AttachMeshData(EntityID id, World* world, const MeshData& data) {
        auto& mesh = world->GetComponent<MeshComponent>(id);
        mesh.vertexCount = (UINT)data.vertices.size();
        mesh.indexCount = (UINT)data.indices.size();
        mesh.stride = sizeof(Vertex);

        Graphics* g = Game::GetInstance()->GetGraphics();
        g->CreateVertexBuffer(data.vertices, mesh.pVertexBuffer.GetAddressOf());
        g->CreateIndexBuffer(data.indices, mesh.pIndexBuffer.GetAddressOf());
    }

    // ★修正箇所: CreateShapeはこれ1つだけにする (古いものは削除)
    // デフォルト引数を Colors::White に設定
    inline void CreateShape(EntityID id, World* world, ShapeType type, DirectX::XMFLOAT4 color = Colors::White) {
        // GeometryGeneratorに色を渡す
        MeshData data = GeometryGenerator::CreateMesh(type, color);
        AttachMeshData(id, world, data);
    }

    // プレイヤーを作成する
    inline EntityID CreatePlayer(World* world,
        float x, float y, float z,
        float rx = 0.0f, float ry = 0.0f, float rz = 0.0f,
        float sx = 1.0f, float sy = 1.0f, float sz = 1.0f) {
        EntityID entity = world->CreateEntity()
            .AddComponent<TransformComponent>(x, y, z, rx, ry, rz, sx, sy, sz)
            .AddComponent<MeshComponent>()
            .AddComponent<PlayerComponent>(5.0f)
            .AddComponent<ColliderComponent>()
            .Build();

        // 基準サイズ
        float baseRadius = 0.5f;
        float baseHeight = 2.0f;

        // コライダーの設定 (カプセル)
        auto& col = world->GetComponent<ColliderComponent>(entity);
        col.SetCapsule(baseRadius * sx, baseHeight * sy);

        // メッシュの設定 (カプセル) - 色: Blue
        CreateShape(entity, world, ShapeType::CAPSULE, Colors::Blue);
        return entity;
    }

    // 敵を作成する
    inline EntityID CreateEnemy(World* world,
        float x, float y, float z,
        float rx = 0.0f, float ry = 0.0f, float rz = 0.0f,
        float sx = 1.0f, float sy = 1.0f, float sz = 1.0f) {
        EntityID entity = world->CreateEntity()
            .AddComponent<TransformComponent>(
                x, y, z,
                rx, ry, rz,
                sx, sy, sz)
            .AddComponent<MeshComponent>()
            .AddComponent<ColliderComponent>()
            .Build();

        // コライダーの設定 (ボックス)
        auto& col = world->GetComponent<ColliderComponent>(entity);
        col.SetBox(1.0f * sx, 1.0f * sy, 1.0f * sz);

        // メッシュの設定 (キューブ) - 色: Yellow
        CreateShape(entity, world, ShapeType::CUBE, Colors::Yellow);

        return entity;
    }

    // 敵を作成する
    inline EntityID CreateGround(World* world,
        float x, float y, float z,
        float rx = 0.0f, float ry = 0.0f, float rz = 0.0f,
        float sx = 1.0f, float sy = 1.0f, float sz = 1.0f) {
        EntityID entity = world->CreateEntity()
            .AddComponent<TransformComponent>(
                x, y, z,
                rx, ry, rz, 
                sx, sy, sz)
            .AddComponent<MeshComponent>()
            .AddComponent<ColliderComponent>()
            .Build();

        // コライダーの設定 (ボックス)
        auto& col = world->GetComponent<ColliderComponent>(entity);
        col.SetBox(1.0f * sx, 1.0f * sy, 1.0f * sz);

        // メッシュの設定 (キューブ) - 色: Yellow
        CreateShape(entity, world, ShapeType::CUBE, Colors::Gray);

        return entity;
    }

    // テスト用の三角形を作成する
    inline EntityID CreateTestTriangle(World* world, float x, float y, float z) {
        EntityID entity = world->CreateEntity()
            .AddComponent<TransformComponent>(x, y, z)
            .AddComponent<MeshComponent>() // 空のメッシュを追加
            .Build();

        // 頂点データの作成（虹色の三角形）
        std::vector<Vertex> vertices = {
            // 位置(x,y,z)           色(r,g,b,a)
            {  0.0f,  0.5f, 0.0f,    1.0f, 0.0f, 0.0f, 1.0f }, // 上：赤
            {  0.5f, -0.5f, 0.0f,    0.0f, 1.0f, 0.0f, 1.0f }, // 右下：緑
            { -0.5f, -0.5f, 0.0f,    0.0f, 0.0f, 1.0f, 1.0f }  // 左下：青
        };

        // メッシュコンポーネントにデータを流し込む
        auto& mesh = world->GetComponent<MeshComponent>(entity);
        mesh.vertexCount = static_cast<UINT>(vertices.size());
        mesh.stride = sizeof(Vertex);

        // Graphicsクラスを使ってGPUバッファを作成
        Game::GetInstance()->GetGraphics()->CreateVertexBuffer(
            vertices,
            mesh.pVertexBuffer.GetAddressOf()
        );

        return entity;
    }
}