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
            world->AddComponent<ActionComponent>(id, ActionComponent{ .attackCooldown = 2.0f, .duration = 0.5f });
            //役割タグ
            if (params.role == PlayerRole::Attacker) {
                world->AddComponent<AttackerTag>(id);//攻撃タグ
            }
            else if (params.role == PlayerRole::Healer) {
                world->AddComponent<HealerTag>(id);//回復タグ
                transformData.position = { 0.0f, -50.0f, 0.0f };
                // Entity作成時のデータを更新
                world->GetComponent<TransformComponent>(id).position = transformData.position;

                // 最初は非アクティブにしておく
                world->GetComponent<PlayerComponent>(id).isActive = false;
            }
            float bodyBaseY = 0.0f;
            // 本体は小さな球（コア）
            AttachMeshAndCollider(id, world, ShapeType::SPHERE, { 0.0f, 1.0f, 1.0f, 1.0f }, ColliderType::Type_Sphere, 0.5f, 0.0f, 0.0f);

            // ====================================================
            // 2. 付属パーツの生成 (Head, Body, Arms...)
            // ====================================================
            DirectX::XMFLOAT4 mainColor = { 0.1f, 0.1f, 0.2f, 1.0f }; // ダークネイビー
            DirectX::XMFLOAT4 subColor = { 0.8f, 0.8f, 0.9f, 1.0f }; // シルバー
            DirectX::XMFLOAT4 glowColor = { 0.0f, 0.8f, 1.0f, 1.0f }; // シアン発光
            DirectX::XMFLOAT4 jointColor = { 0.2f, 0.2f, 0.2f, 1.0f }; // 関節グレー
            
            // ====================================================
            // 1. 頭部 (三角錐 + 双四角錐の耳)
            // ====================================================
            // 頭：三角錐 (頂点を前に向ける => X軸90度回転)
            {
                PlayerPartComponent partData;
                partData.parentID = (int)id;
                partData.partType = PartType::Head;
                partData.baseOffset = { 0.0f,bodyBaseY + 0.9f, 0.0f };
                partData.baseRotation = { DirectX::XM_PIDIV2, 0.0f, 0.0f };

                EntityID headID = world->CreateEntity()
                    .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.6f, 0.8f, 0.6f} })
                    .AddComponent<MeshComponent>()
                    .AddComponent<PlayerPartComponent>(partData)
                    .Build();
                AttachMeshAndCollider(headID, world, ShapeType::TETRAHEDRON, subColor, ColliderType::Type_None, 0.0f, 0.0f, 0.0f);
                // 耳 (ループ展開)
                // 左耳
                {
                    PlayerPartComponent earPart;
                    earPart.parentID = (int)id;
                    earPart.partType = PartType::EarLeft;
                    earPart.baseOffset = { -0.25f, bodyBaseY + 1.3f, -0.1f };
                    earPart.baseRotation = { -0.4f, 0.0f, 0.3f }; // 左に開く

                    EntityID earL = world->CreateEntity()
                        .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.15f, 0.5f, 0.15f} })
                        .AddComponent<MeshComponent>()
                        .AddComponent<PlayerPartComponent>(earPart)
                        .Build();
                    AttachMeshAndCollider(earL, world, ShapeType::DOUBLE_PYRAMID, glowColor, ColliderType::Type_None, 0.0f, 0.0f, 0.0f);
                }
                // 右耳
                {
                    PlayerPartComponent earPart;
                    earPart.parentID = (int)id;
                    earPart.partType = PartType::EarRight;
                    earPart.baseOffset = { 0.25f, bodyBaseY + 1.3f, -0.1f };
                    earPart.baseRotation = { -0.4f, 0.0f, -0.3f }; // 右に開く

                    EntityID earR = world->CreateEntity()
                        .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.15f, 0.5f, 0.15f} })
                        .AddComponent<MeshComponent>()
                        .AddComponent<PlayerPartComponent>(earPart)
                        .Build();
                    AttachMeshAndCollider(earR, world, ShapeType::DOUBLE_PYRAMID, glowColor, ColliderType::Type_None, 0.0f, 0.0f, 0.0f);
                }
            }

            // ====================================================
            // 2. 胴体構成 (コアを中心に上下左右から三角錐)
            // ====================================================
            // 胴体上部：六角錐
            float coreDist = 0.6f; // コアからの距離
            float torsoScale = 0.5f;
            {
                PlayerPartComponent part;
                part.parentID = (int)id;
                part.partType = PartType::Body;
                part.baseOffset = { 0.0f, bodyBaseY + coreDist, 0.0f };
                // X軸180度回転で下を向く
                part.baseRotation = { DirectX::XM_PI, 0.0f, 0.0f };

                EntityID bodyTop = world->CreateEntity()
                    .AddComponent<TransformComponent>(TransformComponent{ .scale = {torsoScale, torsoScale, torsoScale} })
                    .AddComponent<MeshComponent>()
                    .AddComponent<PlayerPartComponent>(part)
                    .Build();
                AttachMeshAndCollider(bodyTop, world, ShapeType::TETRAHEDRON, mainColor, ColliderType::Type_None, 0.0f, 0.0f, 0.0f);
            }

            // 2-2. 下の三角錐 (頂点を上に向ける => コアへ)
            {
                PlayerPartComponent part;
                part.parentID = (int)id;
                part.partType = PartType::Body;
                part.baseOffset = { 0.0f, bodyBaseY - coreDist, 0.0f };
                // デフォルトで上向きなので回転なし
                part.baseRotation = { 0.0f, 0.0f, 0.0f };

                EntityID bodyBottom = world->CreateEntity()
                    .AddComponent<TransformComponent>(TransformComponent{ .scale = {torsoScale, torsoScale, torsoScale} })
                    .AddComponent<MeshComponent>()
                    .AddComponent<PlayerPartComponent>(part)
                    .Build();
                AttachMeshAndCollider(bodyBottom, world, ShapeType::TETRAHEDRON, mainColor, ColliderType::Type_None, 0.0f, 0.0f, 0.0f);
            }

            // 2-3. 右の三角錐 (頂点を左に向ける => コアへ)
            {
                PlayerPartComponent part;
                part.parentID = (int)id;
                part.partType = PartType::Body;
                part.baseOffset = { coreDist, bodyBaseY, 0.0f };
                // Z軸+90度回転で左(-X)を向く
                part.baseRotation = { 0.0f, 0.0f, DirectX::XM_PIDIV2 };

                EntityID bodyRight = world->CreateEntity()
                    .AddComponent<TransformComponent>(TransformComponent{ .scale = {torsoScale, torsoScale, torsoScale} })
                    .AddComponent<MeshComponent>()
                    .AddComponent<PlayerPartComponent>(part)
                    .Build();
                AttachMeshAndCollider(bodyRight, world, ShapeType::TETRAHEDRON, mainColor, ColliderType::Type_None, 0.0f, 0.0f, 0.0f);
            }

            // 2-4. 左の三角錐 (頂点を右に向ける => コアへ)
            {
                PlayerPartComponent part;
                part.parentID = (int)id;
                part.partType = PartType::Body;
                part.baseOffset = { -coreDist, bodyBaseY, 0.0f };
                // Z軸-90度回転で右(+X)を向く
                part.baseRotation = { 0.0f, 0.0f, -DirectX::XM_PIDIV2 };

                EntityID bodyLeft = world->CreateEntity()
                    .AddComponent<TransformComponent>(TransformComponent{ .scale = {torsoScale, torsoScale, torsoScale} })
                    .AddComponent<MeshComponent>()
                    .AddComponent<PlayerPartComponent>(part)
                    .Build();
                AttachMeshAndCollider(bodyLeft, world, ShapeType::TETRAHEDRON, mainColor, ColliderType::Type_None, 0.0f, 0.0f, 0.0f);
            }

            // ====================================================
            // 4. 腕 (肩[トーラス] -> 上腕[三角錐] -> 肘[球] -> 前腕[三角錐])
            // ====================================================
            float shoulderDist = 0.7f;
            float elbowDist = 1.6f;
            float armY = bodyBaseY + 1.2f;

            // --- 左腕 ---
            {
                // 肩
                {
                    PlayerPartComponent p;
                    p.parentID = (int)id;
                    p.partType = PartType::ShoulderLeft;
                    p.baseOffset = { -shoulderDist, armY - 0.7f, 0.0f };
                    p.baseRotation = { 0.0f, 0.0f, -DirectX::XM_PIDIV2 }; // 縦向き

                    EntityID ent = world->CreateEntity()
                        .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.25f, 0.25f, 0.25f} })
                        .AddComponent<MeshComponent>()
                        .AddComponent<PlayerPartComponent>(p)
                        .Build();
                    AttachMeshAndCollider(ent, world, ShapeType::TORUS, mainColor, ColliderType::Type_None, 0.0f, 0.0f, 0.0f);
                }
                // 肘
                {
                    PlayerPartComponent p;
                    p.parentID = (int)id;
                    p.partType = PartType::ArmLeft;
                    p.baseOffset = { -elbowDist + 0.2f, armY - 0.7f, 0.0f };
                    p.baseRotation = { 0.0f, 0.0f, 0.0f };

                    EntityID ent = world->CreateEntity()
                        .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.25f, 0.25f, 0.25f} })
                        .AddComponent<MeshComponent>()
                        .AddComponent<PlayerPartComponent>(p)
                        .Build();
                    AttachMeshAndCollider(ent, world, ShapeType::SPHERE, jointColor, ColliderType::Type_None, 0.0f, 0.0f, 0.0f);
                }
                // 上腕 (左向き=Z軸-90度)
                {
                    PlayerPartComponent p;
                    p.parentID = (int)id;
                    p.partType = PartType::ArmLeft;
                    p.baseOffset = { -elbowDist + 0.4f, armY - 0.7f, 0.0f };
                    p.baseRotation = { 0.0f, 0.0f, -DirectX::XM_PIDIV2 };

                    EntityID ent = world->CreateEntity()
                        .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.2f, 0.5f, 0.2f} })
                        .AddComponent<MeshComponent>()
                        .AddComponent<PlayerPartComponent>(p)
                        .Build();
                    AttachMeshAndCollider(ent, world, ShapeType::TETRAHEDRON, subColor, ColliderType::Type_None, 0.0f, 0.0f, 0.0f);
                }
                // 前腕 (右向き=Z軸+90度)
                {
                    PlayerPartComponent p;
                    p.parentID = (int)id;
                    p.partType = PartType::ArmLeft;
                    p.baseOffset = { -elbowDist, armY - 0.7f, 0.0f };
                    p.baseRotation = { 0.0f, 0.0f, DirectX::XM_PIDIV2 };

                    EntityID ent = world->CreateEntity()
                        .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.25f, 1.0f, 0.25f} })
                        .AddComponent<MeshComponent>()
                        .AddComponent<PlayerPartComponent>(p)
                        .Build();
                    AttachMeshAndCollider(ent, world, ShapeType::TETRAHEDRON, subColor, ColliderType::Type_None, 0.0f, 0.0f, 0.0f);
                }
            }

            // --- 右腕 ---
            {
                // 肩
                {
                    PlayerPartComponent p;
                    p.parentID = (int)id;
                    p.partType = PartType::ShoulderRight;
                    p.baseOffset = { shoulderDist, armY - 0.7f, 0.0f };
                    p.baseRotation = { 0.0f, 0.0f, DirectX::XM_PIDIV2 }; // 縦向き

                    EntityID ent = world->CreateEntity()
                        .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.25f, 0.25f, 0.25f} })
                        .AddComponent<MeshComponent>()
                        .AddComponent<PlayerPartComponent>(p)
                        .Build();
                    AttachMeshAndCollider(ent, world, ShapeType::TORUS, mainColor, ColliderType::Type_None, 0.0f, 0.0f, 0.0f);
                }
                // 肘
                {
                    PlayerPartComponent p;
                    p.parentID = (int)id;
                    p.partType = PartType::ArmRight;
                    p.baseOffset = { elbowDist - 0.2f, armY - 0.7f, 0.0f };
                    p.baseRotation = { 0.0f, 0.0f, 0.0f };

                    EntityID ent = world->CreateEntity()
                        .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.25f, 0.25f, 0.25f} })
                        .AddComponent<MeshComponent>()
                        .AddComponent<PlayerPartComponent>(p)
                        .Build();
                    AttachMeshAndCollider(ent, world, ShapeType::SPHERE, jointColor, ColliderType::Type_None, 0.0f, 0.0f, 0.0f);
                }
                // 上腕 (右向き=Z軸+90度)
                {
                    PlayerPartComponent p;
                    p.parentID = (int)id;
                    p.partType = PartType::ArmRight;
                    p.baseOffset = { elbowDist - 0.4f, armY - 0.7f, 0.0f };
                    p.baseRotation = { 0.0f, 0.0f, DirectX::XM_PIDIV2 };

                    EntityID ent = world->CreateEntity()
                        .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.2f, 0.5f, 0.2f} })
                        .AddComponent<MeshComponent>()
                        .AddComponent<PlayerPartComponent>(p)
                        .Build();
                    AttachMeshAndCollider(ent, world, ShapeType::TETRAHEDRON, subColor, ColliderType::Type_None, 0.0f, 0.0f, 0.0f);
                }
                // 前腕 (左向き=Z軸-90度)
                {
                    PlayerPartComponent p;
                    p.parentID = (int)id;
                    p.partType = PartType::ArmRight;
                    p.baseOffset = { elbowDist, armY - 0.7f, 0.0f };
                    p.baseRotation = { 0.0f, 0.0f, -DirectX::XM_PIDIV2 };

                    EntityID ent = world->CreateEntity()
                        .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.25f, 1.0f, 0.25f} })
                        .AddComponent<MeshComponent>()
                        .AddComponent<PlayerPartComponent>(p)
                        .Build();
                    AttachMeshAndCollider(ent, world, ShapeType::TETRAHEDRON, subColor, ColliderType::Type_None, 0.0f, 0.0f, 0.0f);
                }
            }
            // ====================================================
            // 5. 足 (トーラス -> 三角錐)
            // ====================================================
            float legX = 0.4f;
            float thighY = bodyBaseY - 0.6f;
            float shinY = bodyBaseY - 1.2f;

            // --- 左足 ---
            {
                // 腿
                {
                    PlayerPartComponent p;
                    p.parentID = (int)id;
                    p.partType = PartType::LegLeft;
                    p.baseOffset = { -legX, thighY, 0.0f };
                    p.baseRotation = { 0.0f, 0.0f, 0.0f };

                    EntityID ent = world->CreateEntity()
                        .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.4f, 0.4f, 0.4f} })
                        .AddComponent<MeshComponent>()
                        .AddComponent<PlayerPartComponent>(p)
                        .Build();
                    AttachMeshAndCollider(ent, world, ShapeType::TORUS, mainColor, ColliderType::Type_None, 0.0f, 0.0f, 0.0f);
                }
                // すね
                {
                    PlayerPartComponent p;
                    p.parentID = (int)id;
                    p.partType = PartType::LegLeft;
                    p.baseOffset = { -0.4f, shinY + 0.3f, -0.2f };
                    p.baseRotation = { DirectX::XM_PI, 0.0f, 0.0f }; // 下向き

                    EntityID ent = world->CreateEntity()
                        .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.25f, 1.0f, 0.25f} })
                        .AddComponent<MeshComponent>()
                        .AddComponent<PlayerPartComponent>(p)
                        .Build();
                    AttachMeshAndCollider(ent, world, ShapeType::TETRAHEDRON, subColor, ColliderType::Type_None, 0.0f, 0.0f, 0.0f);
                }
            }

            // --- 右足 ---
            {
                // 腿
                {
                    PlayerPartComponent p;
                    p.parentID = (int)id;
                    p.partType = PartType::LegRight;
                    p.baseOffset = { legX, thighY, 0.0f };
                    p.baseRotation = { 0.0f, 0.0f, 0.0f };

                    EntityID ent = world->CreateEntity()
                        .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.4f, 0.4f, 0.4f} })
                        .AddComponent<MeshComponent>()
                        .AddComponent<PlayerPartComponent>(p)
                        .Build();
                    AttachMeshAndCollider(ent, world, ShapeType::TORUS, mainColor, ColliderType::Type_None, 0.0f, 0.0f, 0.0f);
                }
                // すね
                {
                    PlayerPartComponent p;
                    p.parentID = (int)id;
                    p.partType = PartType::LegRight;
                    p.baseOffset = { 0.4f, shinY + 0.3f, -0.2f };
                    p.baseRotation = { DirectX::XM_PI, 0.0f, 0.0f }; // 下向き

                    EntityID ent = world->CreateEntity()
                        .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.25f, 1.0f, 0.25f} })
                        .AddComponent<MeshComponent>()
                        .AddComponent<PlayerPartComponent>(p)
                        .Build();
                    AttachMeshAndCollider(ent, world, ShapeType::TETRAHEDRON, subColor, ColliderType::Type_None, 0.0f, 0.0f, 0.0f);
                }
            }

            DebugLog("[Factory] Created Sci-Fi Floating Player ID: %d", id);
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
        else if (params.type == "Ground" || params.type == "Ground2" || params.type == "Ground3" || params.type == "Ground4") {
            world->AddComponent<MeshComponent>(id);
            world->AddComponent<ColliderComponent>(id);

            DirectX::XMFLOAT4 color = (params.type == "Ground") ? Colors::Gray : Colors::Magenta;
            AttachMeshAndCollider(id, world, ShapeType::CUBE, color, ColliderType::Type_Box, 1.0f, 1.0f, 1.0f);
            DebugLog("[Factory] Created Ground ID: %d", id);
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

        // 、半径0.5
        AttachMeshAndCollider(id, world, ShapeType::SPHERE, Colors::White, ColliderType::Type_Sphere, 0.5f, 0.0f, 0.0f);

        DebugLog("Spawned AttackSphere! ID: %d", id);
    }
    // ★追加: 回復球の生成
    inline void CreateRecoverySphere(World* world, EntityID ownerID, DirectX::XMFLOAT3 pos, int heal) {
        EntityID id = world->CreateEntity()
            .AddComponent<TransformComponent>(TransformComponent{ .position = pos, .scale = {5.0f, 5.0f, 5.0f} })
            .AddComponent<RecoverySphereComponent>(RecoverySphereComponent{
                .ownerID = (int)ownerID,
                .healAmount = heal,
                .lifeTime = 0.6f,
                .currentRadius = 1.0f,
                .maxRadius = 20.0f,
                .expansionSpeed = 20.0f
                })
            .Build();

        // ★追加: 見た目と当たり判定をつける
        world->AddComponent<MeshComponent>(id);
        world->AddComponent<ColliderComponent>(id);

        // 緑の球、半径1.0
        AttachMeshAndCollider(id, world, ShapeType::SPHERE, Colors::Green, ColliderType::Type_Sphere, 0.5f, 0.0f, 0.0f);

        DebugLog("Spawned RecoverySphere! ID: %d", id);
    }
}