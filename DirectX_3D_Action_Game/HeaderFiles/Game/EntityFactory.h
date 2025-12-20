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
#include "ECS/Components/AttackSphereComponent.h"
#include "ECS/Components/RecoverySphereComponent.h"
#include "ECS/Components/RolesComponent.h"
#include "ECS/Components/PlayerPartComponent.h"
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
        else if (colType == ColliderType::Type_Sphere) {
            col.SetSphere(cx); // cxを半径として使う
        }
    }

    // ★統合生成関数
    inline EntityID CreateEntity(World* world, const EntitySpawnParams& params) {
        // 1. ベースEntity作成 & Transform登録
        TransformComponent transformData{
             .position = params.position,
             .rotation = params.rotation,
             .scale = params.scale
        };
        // 2. ベースEntity作成 & Transform登録
        EntityID id = world->CreateEntity()
            .AddComponent<TransformComponent>(transformData) // 作ったデータを渡す
            .Build();

        // 2. タイプ別コンポーネント構成
        if (params.type == "Player") {
            world->AddComponent<MeshComponent>(id);
            world->AddComponent<ColliderComponent>(id);
            world->AddComponent<PlayerComponent>(id, PlayerComponent{ .moveSpeed = 5.0f });
            world->AddComponent<StatusComponent>(id, StatusComponent{ .hp = 100, .maxHp = 100, .attackPower = 5 });
            world->AddComponent<ActionComponent>(id, ActionComponent{ .attackCooldown = 0.2f, .duration = 0.5f });
            //役割タグ
            if (params.role == PlayerRole::Attacker) {
                world->AddComponent<AttackerTag>(id);//攻撃タグ
            }
            else if (params.role == PlayerRole::Healer) {
                world->AddComponent<HealerTag>(id);//回復タグ
            }
            // 本体は小さな球（コア）
            AttachMeshAndCollider(id, world, ShapeType::SPHERE, { 0.0f, 1.0f, 1.0f, 1.0f }, ColliderType::Type_Capsule, 0.5f, 2.0f, 0.0f);

            // ====================================================
            // 2. 付属パーツの生成 (Head, Body, Arms...)
            // ====================================================
            DirectX::XMFLOAT4 bodyColor = { 0.1f, 0.1f, 0.15f, 1.0f }; // 黒鉄色
            DirectX::XMFLOAT4 eyeColor = { 1.0f, 0.0f, 0.3f, 1.0f };  // ネオンピンクの目
            // --- [Head] 三角錐 ---
            // 顔を前に向けるため、X軸で90度回転させて配置
            {
                EntityID headID = world->CreateEntity()
                    .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.6f, 0.6f, 0.6f} })
                    .AddComponent<MeshComponent>()
                    .AddComponent<PlayerPartComponent>(PlayerPartComponent{
                        .parentID = (int)id,
                        .partType = PartType::Head,
                        .baseOffset = {0.0f, 1.6f, 0.0f}, // 少し高い位置
                        .baseRotation = { DirectX::XM_PIDIV2, 0.0f, 0.0f } // 三角錐の頂点を前に向ける回転
                        })
                    .Build();

                AttachMeshAndCollider(headID, world, ShapeType::TETRAHEDRON, eyeColor, ColliderType::Type_Box, 0.0f, 0.0f, 0.0f);
            }

            // --- [Body] 逆三角形（円錐を逆さにするなど） ---
            {
                EntityID bodyID = world->CreateEntity()
                    .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.8f, 1.2f, 0.8f} })
                    .AddComponent<MeshComponent>()
                    .AddComponent<PlayerPartComponent>(PlayerPartComponent{
                        .parentID = (int)id,
                        .partType = PartType::Body,
                        .baseOffset = {0.0f, 0.6f, 0.0f},
                        .baseRotation = { DirectX::XM_PI, 0.0f, 0.0f } // 逆さにする
                        })
                    .Build();
                AttachMeshAndCollider(bodyID, world, ShapeType::CONE, bodyColor, ColliderType::Type_Box, 0.0f, 0.0f, 0.0f);
            }

            // --- [Right Arm] 右腕 (浮遊する箱) ---
            {
                EntityID armRID = world->CreateEntity()
                    .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.25f, 0.7f, 0.25f} })
                    .AddComponent<MeshComponent>()
                    .AddComponent<PlayerPartComponent>(PlayerPartComponent{
                        .parentID = (int)id,
                        .partType = PartType::ArmRight,
                        .baseOffset = {0.8f, 1.2f, 0.3f}, // 右に配置
                        .baseRotation = { 0.2f, 0.0f, -0.2f } // 少し開く
                        })
                    .Build();
                AttachMeshAndCollider(armRID, world, ShapeType::CUBE, bodyColor, ColliderType::Type_Box, 0.0f, 0.0f, 0.0f);
            }

            // --- [Left Arm] 左腕 ---
            {
                EntityID armLID = world->CreateEntity()
                    .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.25f, 0.7f, 0.25f} })
                    .AddComponent<MeshComponent>()
                    .AddComponent<PlayerPartComponent>(PlayerPartComponent{
                        .parentID = (int)id,
                        .partType = PartType::ArmLeft,
                        .baseOffset = {-0.8f, 1.2f, 0.3f}, // 左に配置
                        .baseRotation = { 0.2f, 0.0f, 0.2f }
                        })
                    .Build();
                AttachMeshAndCollider(armLID, world, ShapeType::CUBE, bodyColor, ColliderType::Type_Box, 0.0f, 0.0f, 0.0f);
            }

            // --- [Bits] 背中に浮くビット（オプションパーツ） ---
            // 2つの小さな三角錐を背面に配置
            for (int i = 0; i < 2; ++i) {
                float xDir = (i == 0) ? -1.0f : 1.0f;
                EntityID bitID = world->CreateEntity()
                    .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.3f, 0.3f, 0.3f} })
                    .AddComponent<MeshComponent>()
                    .AddComponent<PlayerPartComponent>(PlayerPartComponent{
                        .parentID = (int)id,
                        .partType = PartType::Body, // 体と同じ動きにする
                        .baseOffset = {xDir * 0.6f, 1.8f, -0.6f}, // 肩の後ろ
                        .baseRotation = { 0.0f, 0.0f, xDir * 0.5f }
                        })
                    .Build();
                // ビットはシアン色
                AttachMeshAndCollider(bitID, world, ShapeType::TETRAHEDRON, { 0.0f, 1.0f, 1.0f, 1.0f }, ColliderType::Type_Box, 0.0f, 0.0f, 0.0f);
            }

            DebugLog("[Factory] Created Sci-Fi Player ID: %d", id);
        }
        else if (params.type == "Enemy") {
            world->AddComponent<MeshComponent>(id);
            world->AddComponent<ColliderComponent>(id);
            world->AddComponent<EnemyComponent>(id, EnemyComponent{ .moveSpeed = 2.0f });
            world->AddComponent<StatusComponent>(id, StatusComponent{ .hp = 10, .maxHp = 10, .attackPower = 20 });

            // キューブ形状 (1x1x1)
            AttachMeshAndCollider(id, world, ShapeType::CUBE, params.color, ColliderType::Type_Box, 1.0f, 1.0f, 1.0f);
           
            DebugLog("[Factory] Created Enemy ID: %d", id);
        }
        else if (params.type == "Enemy2") {
            world->AddComponent<MeshComponent>(id);
            world->AddComponent<ColliderComponent>(id);
            world->AddComponent<EnemyComponent>(id, EnemyComponent{ .moveSpeed = 4.0f });
            world->AddComponent<StatusComponent>(id, StatusComponent{ .hp = 10, .maxHp = 10, .attackPower = 20 });

            // キューブ形状 (1x1x1)
            AttachMeshAndCollider(id, world, ShapeType::CUBE, params.color, ColliderType::Type_Box, 1.0f, 1.0f, 1.0f);
            /* AttachMeshAndCollider(id, world, ShapeType::CUBE, Colors::Yellow, ColliderType::Type_Box, 1.0f * params.scale.x, 1.0f * params.scale.y, 1.0f * params.scale.z);*/

            DebugLog("[Factory] Created Enemy2 ID: %d", id);
        }
        //else if (params.type == "Ground") {
        //    world->AddComponent<MeshComponent>(id);
        //    world->AddComponent<ColliderComponent>(id);

        //    // 床用キューブ (グレー)
        //    AttachMeshAndCollider(id, world, ShapeType::CUBE, Colors::Gray, ColliderType::Type_Box, 1.0f, 1.0f, 1.0f);
        //  /*  AttachMeshAndCollider(id, world, ShapeType::CUBE, Colors::Gray, ColliderType::Type_Box, 1.0f * params.scale.x, 1.0f * params.scale.y, 1.0f * params.scale.z);*/
        //    DebugLog("[Factory] Created Ground ID: %d", id);
        //}
        else if (params.type == "Ground" || params.type == "Ground2" || params.type == "Ground3" || params.type == "Ground4") {
            world->AddComponent<MeshComponent>(id);
            world->AddComponent<ColliderComponent>(id);

            DirectX::XMFLOAT4 color = (params.type == "Ground") ? Colors::Gray : Colors::Magenta;
            AttachMeshAndCollider(id, world, ShapeType::CUBE, color, ColliderType::Type_Box, 1.0f, 1.0f, 1.0f);
            DebugLog("[Factory] Created Ground ID: %d", id);
        }
        //else if (params.type == "Ground2") {
        //    world->AddComponent<MeshComponent>(id);
        //    world->AddComponent<ColliderComponent>(id);

        //    // 床用キューブ (グレー)
        //    AttachMeshAndCollider(id, world, ShapeType::CUBE, Colors::Magenta, ColliderType::Type_Box, 1.0f, 1.0f, 1.0f);
        //    /*  AttachMeshAndCollider(id, world, ShapeType::CUBE, Colors::Gray, ColliderType::Type_Box, 1.0f * params.scale.x, 1.0f * params.scale.y, 1.0f * params.scale.z);*/
        //    DebugLog("[Factory] Created Ground2 ID: %d", id);
        //}
        //else if (params.type == "Ground3") {
        //    world->AddComponent<MeshComponent>(id);
        //    world->AddComponent<ColliderComponent>(id);

        //    // 床用キューブ (グレー)
        //    AttachMeshAndCollider(id, world, ShapeType::CUBE, Colors::Magenta, ColliderType::Type_Box, 1.0f, 1.0f, 1.0f);
        //    /*  AttachMeshAndCollider(id, world, ShapeType::CUBE, Colors::Gray, ColliderType::Type_Box, 1.0f * params.scale.x, 1.0f * params.scale.y, 1.0f * params.scale.z);*/
        //    DebugLog("[Factory] Created Ground3 ID: %d", id);
        //}
        //else if (params.type == "Ground4") {
        //    world->AddComponent<MeshComponent>(id);
        //    world->AddComponent<ColliderComponent>(id);

        //    // 床用キューブ (グレー)
        //    AttachMeshAndCollider(id, world, ShapeType::CUBE, Colors::Magenta, ColliderType::Type_Box, 1.0f, 1.0f, 1.0f);
        //    /*  AttachMeshAndCollider(id, world, ShapeType::CUBE, Colors::Gray, ColliderType::Type_Box, 1.0f * params.scale.x, 1.0f * params.scale.y, 1.0f * params.scale.z);*/
        //    DebugLog("[Factory] Created Ground4 ID: %d", id);
        //}
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
            .AddComponent<TransformComponent>(TransformComponent{ .position = pos, .scale = scale })
            .AddComponent<ColliderComponent>()
            .AddComponent<AttackBoxComponent>(AttackBoxComponent{ .ownerID = (int)ownerID, .damage = damage, .lifeTime = 0.1f })
            .Build();

        auto& col = world->GetComponent<ColliderComponent>(id);
        col.SetBox(1.0f, 1.0f, 1.0f);

        world->AddComponent<MeshComponent>(id);
        AttachMeshAndCollider(id, world, ShapeType::CUBE, Colors::Red, ColliderType::Type_Box, 1.0f, 1.0f, 1.0f);
        DebugLog("Spawned AttackBox! ID: %d", id);
    }
    // 回復判定作成関数
    inline void CreateRecoveryHitbox(World* world, EntityID ownerID, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 scale, int healAmount) {
        EntityID id = world->CreateEntity()
            .AddComponent<TransformComponent>(TransformComponent{ .position = pos, .scale = scale })
            .AddComponent<ColliderComponent>()
            .AddComponent<RecoveryBoxComponent>(RecoveryBoxComponent{ .ownerID = (int)ownerID, .healAmount = healAmount, .lifeTime = 0.5f })
            .Build();

        auto& col = world->GetComponent<ColliderComponent>(id);
        col.SetBox(1.0f, 1.0f, 1.0f);

        world->AddComponent<MeshComponent>(id);
        AttachMeshAndCollider(id, world, ShapeType::CUBE, Colors::Green, ColliderType::Type_Box, 1.0f, 1.0f, 1.0f);
        DebugLog("Spawned RecoveryBox! ID: %d", id);
    }
    // ★修正: 攻撃球の生成 (MeshとColliderを追加)
    inline void CreateAttackSphere(World* world, EntityID ownerID, DirectX::XMFLOAT3 pos, int damage) {
        EntityID id = world->CreateEntity()
            .AddComponent<TransformComponent>(TransformComponent{ .position = pos, .scale = {0.5f, 0.5f, 0.5f} }) // 初期サイズ
            .AddComponent<AttackSphereComponent>(AttackSphereComponent{
                .ownerID = (int)ownerID,
                .damage = damage,
                .lifeTime = 0.4f,
                .currentRadius = 0.5f,
                .maxRadius = 3.5f,
                .expansionSpeed = 15.0f
                })
            .Build();

        // ★追加: 見た目と当たり判定をつける
        world->AddComponent<MeshComponent>(id);
        world->AddComponent<ColliderComponent>(id);

        // 赤い球、半径0.5
        AttachMeshAndCollider(id, world, ShapeType::SPHERE, Colors::Red, ColliderType::Type_Sphere, 0.5f, 0.0f, 0.0f);

        DebugLog("Spawned AttackSphere! ID: %d", id);
    }
    // ★追加: 回復球の生成
    inline void CreateRecoverySphere(World* world, EntityID ownerID, DirectX::XMFLOAT3 pos, int heal) {
        EntityID id = world->CreateEntity()
            .AddComponent<TransformComponent>(TransformComponent{ .position = pos, .scale = {1.0f, 1.0f, 1.0f} })
            .AddComponent<RecoverySphereComponent>(RecoverySphereComponent{
                .ownerID = (int)ownerID,
                .healAmount = heal,
                .lifeTime = 0.6f,
                .currentRadius = 1.0f,
                .maxRadius = 6.0f,
                .expansionSpeed = 20.0f
                })
            .Build();

        // ★追加: 見た目と当たり判定をつける
        world->AddComponent<MeshComponent>(id);
        world->AddComponent<ColliderComponent>(id);

        // 緑の球、半径1.0
        AttachMeshAndCollider(id, world, ShapeType::SPHERE, Colors::Green, ColliderType::Type_Sphere, 1.0f, 0.0f, 0.0f);

        DebugLog("Spawned RecoverySphere! ID: %d", id);
    }
}