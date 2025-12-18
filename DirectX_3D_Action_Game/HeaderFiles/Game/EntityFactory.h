/*===================================================================
//ファイル:EntityFactory.h
//概要:Entityの生成を管理するファクトリー（共通化対応版）
=====================================================================*/
#pragma once
#include "App/Main.h"
#include "ECS/World.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/CameraComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/EnemyComponent.h"
#include "ECS/Components/ColliderComponent.h"
#include "ECS/Components/StatusComponent.h"
#include "ECS/Components/ActionComponent.h"
#include "ECS/Components/AttackBoxComponent.h"
#include "ECS/Components/RecoveryBoxComponent.h"
#include "ECS/Components/RolesComponent.h"
#include "App/Game.h"
#include "Engine/GeometryGenerator.h"
#include "Engine/Vertex.h"
#include "Engine/Colors.h"
#include <vector>
#include <string>
#include <iostream>
#include <DirectXMath.h>

//役割の種類
enum class PlayerRole {
    Attacker,//攻撃役
    Healer//回復役
};

// 生成パラメータ構造体
struct EntitySpawnParams {
    std::string type;           // "Player", "Enemy", "Ground", "Camera" 等
    DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 rotation = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

    PlayerRole role = PlayerRole::Attacker;

    // オプション（必要に応じて拡張）
    std::string name = "";      // デバッグ識別用など
};

namespace EntityFactory {

    // 内部ヘルパー: メッシュとコライダーをセットアップする
    inline void AttachMeshAndCollider(EntityID id, World* world, ShapeType shape, DirectX::XMFLOAT4 color, ColliderType colType, float cx, float cy, float cz) {
        // メッシュ生成
        MeshData data = GeometryGenerator::CreateMesh(shape, color);

        // MeshComponent設定
        auto& mesh = world->GetComponent<MeshComponent>(id);
        mesh.vertexCount = (UINT)data.vertices.size();
        mesh.indexCount = (UINT)data.indices.size();
        mesh.stride = sizeof(Vertex);

        // GPUバッファ作成
        Graphics* g = Game::GetInstance()->GetGraphics();
        g->CreateVertexBuffer(data.vertices, mesh.pVertexBuffer.GetAddressOf());
        g->CreateIndexBuffer(data.indices, mesh.pIndexBuffer.GetAddressOf());

        // ColliderComponent設定
        auto& col = world->GetComponent<ColliderComponent>(id);
        if (colType == ColliderType::Type_Box) {
            col.SetBox(cx, cy, cz);
        }
        else if (colType == ColliderType::Type_Capsule) {
            // カプセルの場合、cxを半径、cyを高さとして扱う規約にする
            col.SetCapsule(cx, cy);
        }
    }

    // ★統合生成関数
    inline EntityID CreateEntity(World* world, const EntitySpawnParams& params) {
        // 1. ベースEntity作成 & Transform登録
        EntityID id = world->CreateEntity()
            .AddComponent<TransformComponent>(
                params.position.x, params.position.y, params.position.z,
                params.rotation.x, params.rotation.y, params.rotation.z,
                params.scale.x, params.scale.y, params.scale.z
            )
            .Build();

        // 2. タイプ別コンポーネント構成
        if (params.type == "Player") {
            world->AddComponent<MeshComponent>(id);
            world->AddComponent<ColliderComponent>(id);
            world->AddComponent<PlayerComponent>(id, 5.0f); // moveSpeed
            world->AddComponent<StatusComponent>(id, 100, 5);
            world->AddComponent<ActionComponent>(id, 0.2f, 0.5f);
            //役割タグ
            if (params.role == PlayerRole::Attacker) {
                world->AddComponent<AttackerTag>(id);//攻撃タグ
            }
            else if (params.role == PlayerRole::Healer) {
                world->AddComponent<HealerTag>(id);//回復タグ
            }
            // カプセル形状 (半径0.5, 高さ2.0)
            // Scaleを反映させる: 半径はXスケール、高さはYスケールを基準にする
            AttachMeshAndCollider(id, world, ShapeType::CAPSULE,params.color, ColliderType::Type_Capsule, 0.5f, 2.0f, 0.0f);
           
            DebugLog("[Factory] Created Player ID: %d", id);
        }
        else if (params.type == "Enemy") {
            world->AddComponent<MeshComponent>(id);
            world->AddComponent<ColliderComponent>(id);
            world->AddComponent<EnemyComponent>(id, 2.0f);
            world->AddComponent<StatusComponent>(id, 10, 0);

            // キューブ形状 (1x1x1)
            AttachMeshAndCollider(id, world, ShapeType::CUBE, params.color, ColliderType::Type_Box, 1.0f, 1.0f, 1.0f);
           
            DebugLog("[Factory] Created Enemy ID: %d", id);
        }
        else if (params.type == "Enemy2") {
            world->AddComponent<MeshComponent>(id);
            world->AddComponent<ColliderComponent>(id);
            world->AddComponent<EnemyComponent>(id, 4.0f);
            world->AddComponent<StatusComponent>(id, 10, 0);

            // キューブ形状 (1x1x1)
            AttachMeshAndCollider(id, world, ShapeType::CUBE, params.color, ColliderType::Type_Box, 1.0f, 1.0f, 1.0f);
            /* AttachMeshAndCollider(id, world, ShapeType::CUBE, Colors::Yellow, ColliderType::Type_Box, 1.0f * params.scale.x, 1.0f * params.scale.y, 1.0f * params.scale.z);*/

            DebugLog("[Factory] Created Enemy2 ID: %d", id);
        }
        else if (params.type == "Ground") {
            world->AddComponent<MeshComponent>(id);
            world->AddComponent<ColliderComponent>(id);

            // 床用キューブ (グレー)
            AttachMeshAndCollider(id, world, ShapeType::CUBE, Colors::Gray, ColliderType::Type_Box, 1.0f, 1.0f, 1.0f);
          /*  AttachMeshAndCollider(id, world, ShapeType::CUBE, Colors::Gray, ColliderType::Type_Box, 1.0f * params.scale.x, 1.0f * params.scale.y, 1.0f * params.scale.z);*/
            DebugLog("[Factory] Created Ground ID: %d", id);
        }
        else if (params.type == "Ground2") {
            world->AddComponent<MeshComponent>(id);
            world->AddComponent<ColliderComponent>(id);

            // 床用キューブ (グレー)
            AttachMeshAndCollider(id, world, ShapeType::CUBE, Colors::Magenta, ColliderType::Type_Box, 1.0f, 1.0f, 1.0f);
            /*  AttachMeshAndCollider(id, world, ShapeType::CUBE, Colors::Gray, ColliderType::Type_Box, 1.0f * params.scale.x, 1.0f * params.scale.y, 1.0f * params.scale.z);*/
            DebugLog("[Factory] Created Ground2 ID: %d", id);
        }
        else if (params.type == "Ground3") {
            world->AddComponent<MeshComponent>(id);
            world->AddComponent<ColliderComponent>(id);

            // 床用キューブ (グレー)
            AttachMeshAndCollider(id, world, ShapeType::CUBE, Colors::Magenta, ColliderType::Type_Box, 1.0f, 1.0f, 1.0f);
            /*  AttachMeshAndCollider(id, world, ShapeType::CUBE, Colors::Gray, ColliderType::Type_Box, 1.0f * params.scale.x, 1.0f * params.scale.y, 1.0f * params.scale.z);*/
            DebugLog("[Factory] Created Ground3 ID: %d", id);
        }
        else if (params.type == "Ground4") {
            world->AddComponent<MeshComponent>(id);
            world->AddComponent<ColliderComponent>(id);

            // 床用キューブ (グレー)
            AttachMeshAndCollider(id, world, ShapeType::CUBE, Colors::Magenta, ColliderType::Type_Box, 1.0f, 1.0f, 1.0f);
            /*  AttachMeshAndCollider(id, world, ShapeType::CUBE, Colors::Gray, ColliderType::Type_Box, 1.0f * params.scale.x, 1.0f * params.scale.y, 1.0f * params.scale.z);*/
            DebugLog("[Factory] Created Ground4 ID: %d", id);
        }
        else if (params.type == "Camera") {
            world->AddComponent<CameraComponent>(id);
            // カメラ固有の初期化が必要ならここで行う
            DebugLog("[Factory] Created Camera ID: %d", id);
        }
        else {
            DebugLog("[Warning] Unknown Entity Type: %s", params.type.c_str());
        }

        return id;
    }
    //攻撃判定作成関数
    inline void CreateAttackHitbox(World* world, EntityID ownerID, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 scale, int damage) {
        EntityID id = world->CreateEntity()
            .AddComponent<TransformComponent>(pos.x, pos.y, pos.z, 0.0f, 0.0f, 0.0f, scale.x, scale.y, scale.z)
            .AddComponent<ColliderComponent>()
            .AddComponent<AttackBoxComponent>(ownerID, damage, 0.1f)
            .Build();

        //コライダーのサイズ設定
        auto& col = world->GetComponent<ColliderComponent>(id);
		col.SetBox(1.0f, 1.0f, 1.0f); // 基本サイズ
        // デバッグ表示用 (赤色の枠線)
        world->AddComponent<MeshComponent>(id);
		AttachMeshAndCollider(id, world, ShapeType::CUBE, Colors::Red, ColliderType::Type_Box, 1.0f, 1.0f, 1.0f);

        DebugLog("Spawned AttackBox! ID: %d", id);
    }
    // 回復判定作成関数
    inline void CreateRecoveryHitbox(World* world, EntityID ownerID, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 scale, int healAmount) {
        EntityID id = world->CreateEntity()
            .AddComponent<TransformComponent>(pos.x, pos.y, pos.z, 0.0f, 0.0f, 0.0f, scale.x, scale.y, scale.z)
            .AddComponent<ColliderComponent>()
            // 回復コンポーネント (寿命0.5秒)
            .AddComponent<RecoveryBoxComponent>(ownerID, healAmount, 0.5f)
            .Build();

        auto& col = world->GetComponent<ColliderComponent>(id);
        col.SetBox(1.0f, 1.0f, 1.0f);

        // デバッグ表示用 (緑色の枠線)
        world->AddComponent<MeshComponent>(id);
        AttachMeshAndCollider(id, world, ShapeType::CUBE, Colors::Green, ColliderType::Type_Box, 1.0f, 1.0f, 1.0f);

        DebugLog("Spawned RecoveryBox! ID: %d", id);
    }
}