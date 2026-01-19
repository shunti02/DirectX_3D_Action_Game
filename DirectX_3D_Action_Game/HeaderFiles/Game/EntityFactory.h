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
#include "ECS/Components/PhysicsComponent.h"
#include "ECS/Components/BulletComponent.h"
#include "ECS/Components/ParticleComponent.h"
#include "ECS/Components/EnemyPartComponent.h"
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
    PlayerType playerType = PlayerType::AssaultStriker;
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
            world->AddComponent<PlayerComponent>(id, PlayerComponent{ .type = params.playerType,.moveSpeed = 5.0f });
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
            // ----------------------------------------------------
            // ★プレイヤータイプに応じたモデル生成
            // ----------------------------------------------------
            PlayerType currentType = params.playerType;

            // ====================================================
            // 2. 付属パーツの生成 (Head, Body, Arms...)
            // ====================================================
            DirectX::XMFLOAT4 mainColor = { 0.1f, 0.1f, 0.2f, 1.0f }; // ダークネイビー
            DirectX::XMFLOAT4 subColor = { 0.8f, 0.8f, 0.9f, 1.0f }; // シルバー
            DirectX::XMFLOAT4 glowColor = { 0.0f, 0.8f, 1.0f, 1.0f }; // シアン発光
            DirectX::XMFLOAT4 jointColor = { 0.2f, 0.2f, 0.2f, 1.0f }; // 関節グレー

            switch (currentType) {

                // ====================================================
                // TYPE A: Assault Striker (既存モデル)
                // ====================================================
            case PlayerType::AssaultStriker:
            {
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

           // DebugLog("[Factory] Created Sci-Fi Floating Player ID: %d", id);
        }
        break;
        // ====================================================
            // TYPE B: Buster Guard (重装甲パワータイプ)
            // ====================================================
            case PlayerType::BusterGuard:
            {
                mainColor = { 1.0f, 0.75f, 0.0f, 1.0f }; // 山吹色
                subColor = { 0.25f, 0.25f, 0.3f, 1.0f }; // ガンメタル

                // [Body Main] 胸部アーマー
                {
                    EntityID p = world->CreateEntity().AddComponent<TransformComponent>(TransformComponent{ .scale = {1.4f, 1.0f, 1.0f} }).AddComponent<MeshComponent>().AddComponent<PlayerPartComponent>(PlayerPartComponent{ .parentID = (int)id, .partType = PartType::Body, .baseOffset = {0, 0, 0} }).Build();
                    AttachMeshAndCollider(p, world, ShapeType::CUBE, mainColor, ColliderType::Type_None, 0, 0, 0);
                }
                // [Body Lower] 腹部
                {
                    EntityID p = world->CreateEntity().AddComponent<TransformComponent>(TransformComponent{ .scale = {1.0f, 0.6f, 0.8f} }).AddComponent<MeshComponent>().AddComponent<PlayerPartComponent>(PlayerPartComponent{ .parentID = (int)id, .partType = PartType::Body, .baseOffset = {0, -0.7f, 0} }).Build();
                    AttachMeshAndCollider(p, world, ShapeType::CUBE, subColor, ColliderType::Type_None, 0, 0, 0);
                }
                // [Backpack] 大型ジェネレーター
                {
                    EntityID p = world->CreateEntity().AddComponent<TransformComponent>(TransformComponent{ .scale = {1.2f, 1.2f, 0.6f} }).AddComponent<MeshComponent>().AddComponent<PlayerPartComponent>(PlayerPartComponent{ .parentID = (int)id, .partType = PartType::Body, .baseOffset = {0, 0.2f, 0.7f} }).Build();
                    AttachMeshAndCollider(p, world, ShapeType::CUBE, subColor, ColliderType::Type_None, 0, 0, 0);
                }
                // [Head] フラットヘッド
                {
                    EntityID p = world->CreateEntity().AddComponent<TransformComponent>(TransformComponent{ .scale = {0.6f, 0.4f, 0.6f} }).AddComponent<MeshComponent>().AddComponent<PlayerPartComponent>(PlayerPartComponent{ .parentID = (int)id, .partType = PartType::Head, .baseOffset = {0, 0.8f, 0} }).Build();
                    AttachMeshAndCollider(p, world, ShapeType::CUBE, subColor, ColliderType::Type_None, 0, 0, 0);
                }

                // [Shoulder] 球体ショルダー + スパイク
                float sDist = 1.1f;
                for (int i = -1; i <= 1; i += 2) {
                    // 球体
                    EntityID s = world->CreateEntity().AddComponent<TransformComponent>(TransformComponent{ .scale = {1.0f, 1.0f, 1.0f} }).AddComponent<MeshComponent>().AddComponent<PlayerPartComponent>(PlayerPartComponent{ .parentID = (int)id, .partType = (i == -1 ? PartType::ShoulderLeft : PartType::ShoulderRight), .baseOffset = {i * sDist, 0.4f, 0} }).Build();
                    AttachMeshAndCollider(s, world, ShapeType::SPHERE, mainColor, ColliderType::Type_None, 0, 0, 0);
                    // スパイク
                    EntityID sp = world->CreateEntity().AddComponent<TransformComponent>(TransformComponent{ .scale = {0.3f, 0.6f, 0.3f} }).AddComponent<MeshComponent>().AddComponent<PlayerPartComponent>(PlayerPartComponent{ .parentID = (int)id, .partType = (i == -1 ? PartType::ShoulderLeft : PartType::ShoulderRight), .baseOffset = {i * (sDist + 0.3f), 0.8f, 0}, .baseRotation = {0,0,i * -0.5f} }).Build();
                    AttachMeshAndCollider(sp, world, ShapeType::TETRAHEDRON, subColor, ColliderType::Type_None, 0, 0, 0);
                }

                // [Arm] ハンマーアーム (肘なし、太い腕)
                for (int i = -1; i <= 1; i += 2) {
                    EntityID a = world->CreateEntity().AddComponent<TransformComponent>(TransformComponent{ .scale = {0.6f, 1.2f, 0.6f} }).AddComponent<MeshComponent>().AddComponent<PlayerPartComponent>(PlayerPartComponent{ .parentID = (int)id, .partType = (i == -1 ? PartType::ArmLeft : PartType::ArmRight), .baseOffset = {i * 1.8f, -0.6f, 0}, .baseRotation = {0,0,i * 0.2f} }).Build();
                    AttachMeshAndCollider(a, world, ShapeType::CUBE, subColor, ColliderType::Type_None, 0, 0, 0);
                    // 手首のリング
                    EntityID r = world->CreateEntity().AddComponent<TransformComponent>(TransformComponent{ .scale = {0.7f, 0.2f, 0.7f} }).AddComponent<MeshComponent>().AddComponent<PlayerPartComponent>(PlayerPartComponent{ .parentID = (int)id, .partType = (i == -1 ? PartType::HandLeft : PartType::HandRight), .baseOffset = {i * 1.9f, -1.2f, 0}, .baseRotation = {0,0,i * 0.2f} }).Build();
                    AttachMeshAndCollider(r, world, ShapeType::TORUS, mainColor, ColliderType::Type_None, 0, 0, 0);
                }

                // [Leg] 重厚な脚
                for (int i = -1; i <= 1; i += 2) {
                    EntityID l = world->CreateEntity().AddComponent<TransformComponent>(TransformComponent{ .scale = {0.7f, 1.4f, 0.9f} }).AddComponent<MeshComponent>().AddComponent<PlayerPartComponent>(PlayerPartComponent{ .parentID = (int)id, .partType = (i == -1 ? PartType::LegLeft : PartType::LegRight), .baseOffset = {i * 0.5f, -1.2f, 0} }).Build();
                    AttachMeshAndCollider(l, world, ShapeType::CUBE, subColor, ColliderType::Type_None, 0, 0, 0);
                }
            }
            break;

            // ====================================================
            // TYPE C: Plasma Sniper (高機動タイプ)
            // ====================================================
            case PlayerType::PlasmaSniper:
            {
                mainColor = { 0.0f, 1.0f, 0.6f, 1.0f }; // エメラルドグリーン
                subColor = { 0.9f, 0.9f, 0.9f, 1.0f };  // 白

                // [Frames] 骨格フレーム
                EntityID f1 = world->CreateEntity().AddComponent<TransformComponent>(TransformComponent{ .scale = {0.6f, 0.2f, 0.4f} }).AddComponent<MeshComponent>().AddComponent<PlayerPartComponent>(PlayerPartComponent{ .parentID = (int)id, .partType = PartType::Body, .baseOffset = {0, 0.6f, 0} }).Build();
                AttachMeshAndCollider(f1, world, ShapeType::CUBE, subColor, ColliderType::Type_None, 0, 0, 0);
                EntityID f2 = world->CreateEntity().AddComponent<TransformComponent>(TransformComponent{ .scale = {0.4f, 0.2f, 0.4f} }).AddComponent<MeshComponent>().AddComponent<PlayerPartComponent>(PlayerPartComponent{ .parentID = (int)id, .partType = PartType::Body, .baseOffset = {0, -0.6f, 0} }).Build();
                AttachMeshAndCollider(f2, world, ShapeType::CUBE, subColor, ColliderType::Type_None, 0, 0, 0);
                // 接続パイプ
                EntityID pL = world->CreateEntity().AddComponent<TransformComponent>(TransformComponent{ .scale = {0.1f, 1.2f, 0.1f} }).AddComponent<MeshComponent>().AddComponent<PlayerPartComponent>(PlayerPartComponent{ .parentID = (int)id, .partType = PartType::Body, .baseOffset = {-0.4f, 0, -0.2f} }).Build();
                AttachMeshAndCollider(pL, world, ShapeType::CUBE, mainColor, ColliderType::Type_None, 0, 0, 0);
                EntityID pR = world->CreateEntity().AddComponent<TransformComponent>(TransformComponent{ .scale = {0.1f, 1.2f, 0.1f} }).AddComponent<MeshComponent>().AddComponent<PlayerPartComponent>(PlayerPartComponent{ .parentID = (int)id, .partType = PartType::Body, .baseOffset = {0.4f, 0, -0.2f} }).Build();
                AttachMeshAndCollider(pR, world, ShapeType::CUBE, mainColor, ColliderType::Type_None, 0, 0, 0);

                // [Head] センサーアイ
                EntityID h = world->CreateEntity().AddComponent<TransformComponent>(TransformComponent{ .scale = {0.3f, 0.5f, 0.5f} }).AddComponent<MeshComponent>().AddComponent<PlayerPartComponent>(PlayerPartComponent{ .parentID = (int)id, .partType = PartType::Head, .baseOffset = {0, 0.9f, 0.2f} }).Build();
                AttachMeshAndCollider(h, world, ShapeType::CUBE, mainColor, ColliderType::Type_None, 0, 0, 0);

                // [Wing] 大型バインダー
                for (int i = -1; i <= 1; i += 2) {
                    EntityID w = world->CreateEntity().AddComponent<TransformComponent>(TransformComponent{ .scale = {0.1f, 1.8f, 0.6f} }).AddComponent<MeshComponent>().AddComponent<PlayerPartComponent>(PlayerPartComponent{ .parentID = (int)id, .partType = PartType::Body, .baseOffset = {i * 0.8f, 0.5f, -0.5f}, .baseRotation = {0.6f, 0, i * 0.4f} }).Build();
                    AttachMeshAndCollider(w, world, ShapeType::TETRAHEDRON, mainColor, ColliderType::Type_None, 0, 0, 0);
                }

                // [Arm] 長い腕
                for (int i = -1; i <= 1; i += 2) {
                    EntityID s = world->CreateEntity().AddComponent<TransformComponent>(TransformComponent{ .scale = {0.4f, 0.4f, 0.4f} }).AddComponent<MeshComponent>().AddComponent<PlayerPartComponent>(PlayerPartComponent{ .parentID = (int)id, .partType = (i == -1 ? PartType::ShoulderLeft : PartType::ShoulderRight), .baseOffset = {i * 0.6f, 0.6f, 0} }).Build();
                    AttachMeshAndCollider(s, world, ShapeType::SPHERE, subColor, ColliderType::Type_None, 0, 0, 0);

                    EntityID a = world->CreateEntity().AddComponent<TransformComponent>(TransformComponent{ .scale = {0.15f, 1.4f, 0.15f} }).AddComponent<MeshComponent>().AddComponent<PlayerPartComponent>(PlayerPartComponent{ .parentID = (int)id, .partType = (i == -1 ? PartType::ArmLeft : PartType::ArmRight), .baseOffset = {i * 0.7f, -0.2f, 0}, .baseRotation = {0,0,i * 0.1f} }).Build();
                    AttachMeshAndCollider(a, world, ShapeType::CUBE, mainColor, ColliderType::Type_None, 0, 0, 0);
                }
                // [Leg] 逆関節
                for (int i = -1; i <= 1; i += 2) {
                    EntityID l = world->CreateEntity().AddComponent<TransformComponent>(TransformComponent{ .scale = {0.2f, 1.5f, 0.3f} }).AddComponent<MeshComponent>().AddComponent<PlayerPartComponent>(PlayerPartComponent{ .parentID = (int)id, .partType = (i == -1 ? PartType::LegLeft : PartType::LegRight), .baseOffset = {i * 0.3f, -1.3f, -0.3f}, .baseRotation = {0.4f, 0, 0} }).Build();
                    AttachMeshAndCollider(l, world, ShapeType::TETRAHEDRON, subColor, ColliderType::Type_None, 0, 0, 0);
                }
            }
            break;

            } // end switch
            DebugLog("[Factory] Created Sci-Fi Player ID: %d (Type: %d)", id, (int)currentType);
        }
        // ====================================================
        // ENEMY TYPE 1: 雑魚 (Assault Drone)
        // イメージ: 単眼の突撃ドローン。大きな一つ目と、左右の鋭いウィング。
        // ====================================================
        else if (params.type == "Enemy") {
            world->AddComponent<MeshComponent>(id);
            world->AddComponent<ColliderComponent>(id);
            world->AddComponent<EnemyComponent>(id, EnemyComponent{ .moveSpeed = 3.5f, .attackRange = 1.5f, .isRanged = false, .weight = 1.0f ,.isImmovable = false });
            world->AddComponent<StatusComponent>(id, StatusComponent{ .hp = 30, .maxHp = 30, .attackPower = 10 });
            world->AddComponent<PhysicsComponent>(id, PhysicsComponent{ .velocity = {0,0,0}, .useGravity = true });

            // 本体（コア）
            AttachMeshAndCollider(id, world, ShapeType::SPHERE, { 0.2f, 0.2f, 0.2f, 1.0f }, ColliderType::Type_Sphere, 0.5f, 0, 0);

            // [Head] 単眼 (発光する黄色)
            {
                EntityID eye = world->CreateEntity()
                    .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.3f, 0.3f, 0.3f} })
                    .AddComponent<MeshComponent>()
                    .AddComponent<EnemyPartComponent>(EnemyPartComponent{ .parentID = (int)id, .partType = EnemyPartType::Head, .baseOffset = {0, 0, 0.25f} })
                    .Build();
                AttachMeshAndCollider(eye, world, ShapeType::SPHERE, { 1.0f, 0.8f, 0.0f, 1.0f }, ColliderType::Type_None, 0, 0, 0);
            }
            // [Body] 装甲シェル (上下のプレート)
            {
                EntityID shell = world->CreateEntity()
                    .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.6f, 0.6f, 0.6f} })
                    .AddComponent<MeshComponent>()
                    .AddComponent<EnemyPartComponent>(EnemyPartComponent{ .parentID = (int)id, .partType = EnemyPartType::Body,.baseOffset = {0, 0, 0}})
                    .Build();
                AttachMeshAndCollider(shell, world, ShapeType::DOUBLE_PYRAMID, params.color, ColliderType::Type_None, 0, 0, 0);
            }
            // [Wing] 左ウィング (鋭い刃)
            {
                EntityID wing = world->CreateEntity()
                    .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.1f, 0.8f, 0.4f} })
                    .AddComponent<MeshComponent>()
                    .AddComponent<EnemyPartComponent>(EnemyPartComponent{ .parentID = (int)id, .partType = EnemyPartType::Wing, .baseOffset = {-0.5f, 0.2f, -0.2f}, .baseRotation = {0, 0, 0.5f} })
                    .Build();
                AttachMeshAndCollider(wing, world, ShapeType::TETRAHEDRON, { 0.8f, 0.1f, 0.1f, 1.0f }, ColliderType::Type_None, 0, 0, 0);
            }
            // [Wing] 右ウィング
            {
                EntityID wing = world->CreateEntity()
                    .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.1f, 0.8f, 0.4f} })
                    .AddComponent<MeshComponent>()
                    .AddComponent<EnemyPartComponent>(EnemyPartComponent{ .parentID = (int)id, .partType = EnemyPartType::Wing, .baseOffset = {0.5f, 0.2f, -0.2f}, .baseRotation = {0, 0, -0.5f} })
                    .Build();
                AttachMeshAndCollider(wing, world, ShapeType::TETRAHEDRON, { 0.8f, 0.1f, 0.1f, 1.0f }, ColliderType::Type_None, 0, 0, 0);
            }
            // [Thruster] 後部スラスター
            {
                EntityID th = world->CreateEntity()
                    .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.2f, 0.2f, 0.4f} })
                    .AddComponent<MeshComponent>()
                    .AddComponent<EnemyPartComponent>(EnemyPartComponent{ .parentID = (int)id, .partType = EnemyPartType::Thruster, .baseOffset = {0, -0.2f, -0.5f} })
                    .Build();
                AttachMeshAndCollider(th, world, ShapeType::CUBE, { 0.3f, 0.3f, 0.3f, 1.0f }, ColliderType::Type_None, 0, 0, 0);
            }
            DebugLog("[Factory] Created Assault Drone ID: %d", id);
        }

        // ====================================================
        // ENEMY TYPE 2: 遠距離 (Orbital Sniper)
        // イメージ: 浮遊する十字型の砲台。中央にクリスタル、前方に長いレールガン。
        // ====================================================
        else if (params.type == "EnemyRanged") {
            world->AddComponent<MeshComponent>(id);
            world->AddComponent<ColliderComponent>(id);
            world->AddComponent<EnemyComponent>(id, EnemyComponent{ .moveSpeed = 2.5f, .attackRange = 15.0f, .isRanged = true, .weight = 3.0f,.isImmovable = false });
            world->AddComponent<StatusComponent>(id, StatusComponent{ .hp = 30, .maxHp = 30, .attackPower = 15 });
            world->AddComponent<PhysicsComponent>(id, PhysicsComponent{ .velocity = {0,0,0}, .useGravity = true });

            DirectX::XMFLOAT4 baseColor = { 0.4f, 0.0f, 0.6f, 1.0f }; // 紫
            DirectX::XMFLOAT4 glowColor = { 0.8f, 0.2f, 1.0f, 1.0f }; // 明るい紫

            // 本体（中心核）
            AttachMeshAndCollider(id, world, ShapeType::SPHERE, baseColor, ColliderType::Type_Sphere, 0.6f, 0, 0);

            // [Weapon] 長砲身レールガン
            {
                EntityID cannon = world->CreateEntity()
                    .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.15f, 0.15f, 2.0f} })
                    .AddComponent<MeshComponent>()
                    .AddComponent<EnemyPartComponent>(EnemyPartComponent{ .parentID = (int)id, .partType = EnemyPartType::Weapon, .baseOffset = {0, 0, 0.8f} })
                    .Build();
                AttachMeshAndCollider(cannon, world, ShapeType::CUBE, { 0.1f, 0.1f, 0.1f, 1.0f }, ColliderType::Type_None, 0, 0, 0);
            }
            // [Ring] 回転するリング（縦）
            {
                EntityID ring = world->CreateEntity()
                    .AddComponent<TransformComponent>(TransformComponent{ .scale = {1.0f, 1.0f, 0.1f} })
                    .AddComponent<MeshComponent>()
                    .AddComponent<EnemyPartComponent>(EnemyPartComponent{ .parentID = (int)id, .partType = EnemyPartType::Ring, .baseOffset = {0, 0, 0} })
                    .Build();
                AttachMeshAndCollider(ring, world, ShapeType::TORUS, glowColor, ColliderType::Type_None, 0, 0, 0);
            }
            // [Decoration] 上下の安定翼
            for (int i = -1; i <= 1; i += 2) { // i = -1, 1
                EntityID fin = world->CreateEntity()
                    .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.1f, 0.8f, 0.8f} })
                    .AddComponent<MeshComponent>()
                    .AddComponent<EnemyPartComponent>(EnemyPartComponent{ .parentID = (int)id, .partType = EnemyPartType::Wing, .baseOffset = {0, i * 0.8f, -0.5f} })
                    .Build();
                AttachMeshAndCollider(fin, world, ShapeType::TETRAHEDRON, baseColor, ColliderType::Type_None, 0, 0, 0);
            }
            DebugLog("[Factory] Created Orbital Sniper ID: %d", id);
            }

            // ====================================================
            // ENEMY TYPE 3: 中ボス (Heavy Golem)
            // イメージ: ずんぐりした胴体に、巨大な球体の肩と、太い脚。
            // ====================================================
        else if (params.type == "Enemy2") {
                world->AddComponent<MeshComponent>(id);
                world->AddComponent<ColliderComponent>(id);
                world->AddComponent<EnemyComponent>(id, EnemyComponent{ .moveSpeed = 2.0f, .attackRange = 3.0f, .isRanged = false, .weight = 10.0f,.isImmovable = false });
                world->AddComponent<StatusComponent>(id, StatusComponent{ .hp = 200, .maxHp = 200, .attackPower = 25 });
                world->AddComponent<PhysicsComponent>(id, PhysicsComponent{ .velocity = {0,0,0}, .useGravity = true });

                DirectX::XMFLOAT4 armorColor = { 0.8f, 0.6f, 0.0f, 1.0f }; // 黄土色/金
                DirectX::XMFLOAT4 jointColor = { 0.2f, 0.2f, 0.2f, 1.0f }; // 黒

                // 本体（胸部）
                AttachMeshAndCollider(id, world, ShapeType::CUBE, armorColor, ColliderType::Type_Box, 1.5f, 1.5f, 1.5f);

                // [Head] 頭部（埋まっている感じ）
                {
                    EntityID head = world->CreateEntity()
                        .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.8f, 0.5f, 0.8f} })
                        .AddComponent<MeshComponent>()
                        .AddComponent<EnemyPartComponent>(EnemyPartComponent{ .parentID = (int)id, .partType = EnemyPartType::Head, .baseOffset = {0, 0.9f, 0} })
                        .Build();
                    AttachMeshAndCollider(head, world, ShapeType::CUBE, jointColor, ColliderType::Type_None, 0, 0, 0);
                }
                // [Shoulder] 巨大な肩アーマー (左右)dddddd
                for (int i = -1; i <= 1; i += 2) {
                    EntityID shoulder = world->CreateEntity()
                        .AddComponent<TransformComponent>(TransformComponent{ .scale = {1.5f, 1.5f, 1.5f} })
                        .AddComponent<MeshComponent>()
                        .AddComponent<EnemyPartComponent>(EnemyPartComponent{ .parentID = (int)id, .partType = (i == -1 ? EnemyPartType::ArmLeft : EnemyPartType::ArmRight), .baseOffset = {i * 1.6f, 0.5f, 0.0f} })
                        .Build();
                    AttachMeshAndCollider(shoulder, world, ShapeType::SPHERE, jointColor, ColliderType::Type_None, 0, 0, 0);
                }
                // [Arm] 腕（スパイク）
                for (int i = -1; i <= 1; i += 2) {
                    EntityID arm = world->CreateEntity()
                        .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.5f, 1.2f, 0.5f} })
                        .AddComponent<MeshComponent>()
                        .AddComponent<EnemyPartComponent>(EnemyPartComponent{ .parentID = (int)id, .partType = (i == -1 ? EnemyPartType::ArmLeft : EnemyPartType::ArmRight), .baseOffset = {i * 2.0f, -0.5f, 0.5f}, .baseRotation = {1.0f, 0, 0} })
                        .Build();
                    AttachMeshAndCollider(arm, world, ShapeType::TETRAHEDRON, jointColor, ColliderType::Type_None, 0, 0, 0);
                }
                // [Leg] 太い脚
                for (int i = -1; i <= 1; i += 2) {
                    EntityID leg = world->CreateEntity()
                        .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.6f, 1.2f, 0.8f} })
                        .AddComponent<MeshComponent>()
                        .AddComponent<EnemyPartComponent>(EnemyPartComponent{ .parentID = (int)id, .partType = (i == -1 ? EnemyPartType::LegLeft : EnemyPartType::LegRight), .baseOffset = {i * 0.6f, -1.2f, 0.0f} })
                        .Build();
                    AttachMeshAndCollider(leg, world, ShapeType::CUBE, jointColor, ColliderType::Type_None, 0, 0, 0);
                }
                DebugLog("[Factory] Created Heavy Golem ID: %d", id);
                }

                // ====================================================
                // ENEMY TYPE 4: ラスボス (Crimson Seraph)
                // イメージ: 幾何学的な天使。赤いコアを中心に、複数のリングが回転し、背後には結晶の翼が広がる。
                // ====================================================
        else if (params.type == "Boss") {
                    world->AddComponent<MeshComponent>(id);
                    world->AddComponent<ColliderComponent>(id);
                    // 不動設定 (weight=超大, isImmovable=true)
                    world->AddComponent<EnemyComponent>(id, EnemyComponent{ .moveSpeed = 0.0f, .attackRange = 40.0f, .isRanged = true, .weight = 1000.0f, .isImmovable = true });
                    world->AddComponent<StatusComponent>(id, StatusComponent{ .hp = 1000, .maxHp = 1000, .attackPower = 40 });
                    world->AddComponent<PhysicsComponent>(id, PhysicsComponent{ .velocity = {0,0,0}, .useGravity = false });

                    // コアの色: 赤（ご指定の色）
                    DirectX::XMFLOAT4 coreColor = { 0.8f, 0.0f, 0.0f, 1.0f };
                    DirectX::XMFLOAT4 darkMetal = { 0.2f, 0.05f, 0.05f, 1.0f };
                    DirectX::XMFLOAT4 energy = { 1.0f, 0.2f, 0.2f, 1.0f };

                    // 本体（巨大コア）
                    AttachMeshAndCollider(id, world, ShapeType::SPHERE, coreColor, ColliderType::Type_Sphere, 3.0f, 3.0f, 3.0f);

                    // [Ring 1] 横回転する巨大リング
                    {
                        EntityID ring = world->CreateEntity()
                            .AddComponent<TransformComponent>(TransformComponent{ .scale = {2.0f, 0.2f, 2.0f} }) // 一番大きい
                            .AddComponent<MeshComponent>()
                            // baseRotation.y = 90度 (縦向き)
                            .AddComponent<EnemyPartComponent>(EnemyPartComponent{ .parentID = (int)id, .partType = EnemyPartType::Ring, .baseOffset = {0,0,0}, .baseRotation = {0, 0, 0} })
                            .Build();
                        AttachMeshAndCollider(ring, world, ShapeType::TORUS, darkMetal, ColliderType::Type_None, 0, 0, 0);
                    }
                    // [Ring 2] 縦回転するリング
                    {
                        EntityID ring = world->CreateEntity()
                            .AddComponent<TransformComponent>(TransformComponent{ .scale = {2.0f, 0.2f, 2.0f} }) // 中くらい
                            .AddComponent<MeshComponent>()
                            // baseRotation.x = 90度 (横向き)
                            .AddComponent<EnemyPartComponent>(EnemyPartComponent{ .parentID = (int)id, .partType = EnemyPartType::Ring, .baseOffset = {0,0,0}, .baseRotation = {0, 0, 0} })
                            .Build();
                        AttachMeshAndCollider(ring, world, ShapeType::TORUS, energy, ColliderType::Type_None, 0, 0, 0);
                    }
                    // [Ring 3] 斜め回転するリング
                    {
                        EntityID ring = world->CreateEntity()
                            .AddComponent<TransformComponent>(TransformComponent{ .scale = {2.0f, 0.2f, 2.0f}}) // 小さい
                            .AddComponent<MeshComponent>()
                            // baseRotation = 斜め45度
                            .AddComponent<EnemyPartComponent>(EnemyPartComponent{ .parentID = (int)id, .partType = EnemyPartType::Ring, .baseOffset = {0,0,0}, .baseRotation = {0, 0, 0} })
                            .Build();
                        AttachMeshAndCollider(ring, world, ShapeType::TORUS, darkMetal, ColliderType::Type_None, 0, 0, 0);
                    }

                    // [Wings] 背面の結晶翼 (6枚のブレードを展開)
                    for (int i = 0; i < 6; ++i) {
                        float angleStep = DirectX::XM_PI / 3.0f; // 半円状に広げる
                        float angle = -DirectX::XM_PIDIV2 + (angleStep * i); // -90度からスタート

                        float x = cosf(angle) * 4.0f;
                        float y = sinf(angle) * 4.0f;

                        EntityID wing = world->CreateEntity()
                            .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.5f, 2.5f, 0.2f} })
                            .AddComponent<MeshComponent>()
                            .AddComponent<EnemyPartComponent>(EnemyPartComponent{
                               .parentID = (int)id,
                               .partType = EnemyPartType::Wing,
                               .baseOffset = {x, y + 2.0f, 2.0f},
                               .baseRotation = {0, 0, angle - DirectX::XM_PIDIV2}
                                })
                            .Build();
                        AttachMeshAndCollider(wing, world, ShapeType::DOUBLE_PYRAMID, energy, ColliderType::Type_None, 0, 0, 0);
                    }

                    // [Satellite] 周囲を回るビット (4つ)
                    for (int i = 0; i < 4; ++i) {
                        float angle = (DirectX::XM_2PI / 4.0f) * i;
                        float x = cosf(angle) * 5.0f;
                        float z = sinf(angle) * 5.0f;
                        EntityID bit = world->CreateEntity()
                            .AddComponent<TransformComponent>(TransformComponent{ .scale = {0.8f, 1.5f, 0.8f} })
                            .AddComponent<MeshComponent>()
                            .AddComponent<EnemyPartComponent>(EnemyPartComponent{ .parentID = (int)id, .partType = EnemyPartType::Shield, .baseOffset = {x, 0, z} })
                            .Build();
                        AttachMeshAndCollider(bit, world, ShapeType::DOUBLE_PYRAMID, darkMetal, ColliderType::Type_None, 0, 0, 0);
                    }
                    DebugLog("[Factory] Created Boss (Crimson Seraph) ID: %d", id);
                    }
        else if (params.type == "Ground" || params.type == "Ground2" || params.type == "Ground3" || params.type == "Ground4") {
            world->AddComponent<MeshComponent>(id);
            world->AddComponent<ColliderComponent>(id);

            DirectX::XMFLOAT4 color = (params.type == "Ground") ? Colors::Gray : Colors::Magenta;
            AttachMeshAndCollider(id, world, ShapeType::CUBE, color, ColliderType::Type_Box, 1.0f, 1.0f, 1.0f);
            DebugLog("[Factory] Created Ground ID: %d", id);
        }
        // ★追加: 汎用的な壁 (Wall)
        else if (params.type == "Wall") {
            world->AddComponent<MeshComponent>(id);
            world->AddComponent<ColliderComponent>(id);

            // 指定された色、サイズ、座標で箱を作る
            // ※ColliderType::Type_Box を指定し、サイズは scale に依存させる
            AttachMeshAndCollider(id, world, ShapeType::CUBE, params.color, ColliderType::Type_Box, 1.0f, 1.0f, 1.0f);

            DebugLog("[Factory] Created Wall ID: %d", id);
        }
        else if (params.type == "HealSpot" || params.type == "Cube" || params.type == "Diamond") {
            world->AddComponent<MeshComponent>(id);
            world->AddComponent<ColliderComponent>(id);

            // 形: DOUBLE_PYRAMID (ダイヤ型/クリスタル型)
            // 当たり判定: Type_Box (箱型)
            AttachMeshAndCollider(id, world, ShapeType::DOUBLE_PYRAMID, params.color, ColliderType::Type_Box, 1.0f, 2.0f, 1.0f);

            DebugLog("[Factory] Created Crystal/HealSpot ID: %d", id);
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
    // 攻撃球の生成 (MeshとColliderを追加)
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

        // 見た目と当たり判定をつける
        world->AddComponent<MeshComponent>(id);
        world->AddComponent<ColliderComponent>(id);

        // 、半径0.5
        AttachMeshAndCollider(id, world, ShapeType::SPHERE, Colors::White, ColliderType::Type_Sphere, 0.5f, 0.0f, 0.0f);

        DebugLog("Spawned AttackSphere! ID: %d", id);
    }
    // 回復球の生成
    inline void CreateRecoverySphere(World* world, EntityID ownerID, DirectX::XMFLOAT3 pos, int heal) {
        EntityID id = world->CreateEntity()
            .AddComponent<TransformComponent>(TransformComponent{ .position = pos, .scale = {1.0f, 1.0f, 1.0f} })
            .AddComponent<RecoverySphereComponent>(RecoverySphereComponent{
            // 古いメンバ(ownerID, currentRadius等)を消し、新しいメンバのみにする
            .radius = 2.0f,
            .healAmount = heal,
            .isActive = true,
            .rotationAngle = 0.0f
                })
            .Build();

        // 見た目と当たり判定をつける
        world->AddComponent<MeshComponent>(id);
        world->AddComponent<ColliderComponent>(id);

        // 緑の球、半径0.5 (Scale1.0のとき)
        // ここでは ShapeType::SPHERE (半径1) なので Scaleを調整
        AttachMeshAndCollider(id, world, ShapeType::SPHERE, Colors::Green, ColliderType::Type_Sphere, 1.0f, 0.0f, 0.0f);

        DebugLog("Spawned RecoverySphere (Item)! ID: %d", id);
    }
    // ★追加: 敵の弾を生成
    inline void CreateEnemyBullet(World* world, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 dir, int damage) {
        // 小さな赤い球
        EntityID id = world->CreateEntity()
            .AddComponent<TransformComponent>(TransformComponent{ .position = pos, .scale = {0.3f, 0.3f, 0.3f} })
            .AddComponent<BulletComponent>(BulletComponent{ .damage = damage, .lifeTime = 5.0f, .isActive = true })
            .Build();

        world->AddComponent<MeshComponent>(id);
        world->AddComponent<ColliderComponent>(id);

        // メッシュと当たり判定 (球)
        AttachMeshAndCollider(id, world, ShapeType::SPHERE, Colors::Red, ColliderType::Type_Sphere, 0.3f, 0.0f, 0.0f);

        // 物理挙動 (飛んでいく速度)
        if (!world->GetRegistry()->HasComponent<PhysicsComponent>(id)) {
            // 弾速 (10.0f)
            float speed = 10.0f;
            world->AddComponent<PhysicsComponent>(id, PhysicsComponent{
                .velocity = { dir.x * speed, dir.y * speed, dir.z * speed },
                .useGravity = false // 重力の影響を受けない
                });
        }

        DebugLog("Enemy Fired! ID: %d", id);
    }
    // ★追加: プレイヤーの弾を生成
    inline void CreatePlayerBullet(World* world, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 dir, int damage) {
        // 小さな発光する弾
        EntityID id = world->CreateEntity()
            .AddComponent<TransformComponent>(TransformComponent{ .position = pos, .scale = {0.4f, 0.4f, 0.8f} }) // 少し細長く
            // 定義順: damage, lifeTime, isActive, fromPlayer
            .AddComponent<BulletComponent>(BulletComponent{
                .damage = damage,
                .lifeTime = 3.0f,
                .isActive = true,
                .fromPlayer = true // ★重要: プレイヤーの弾
                })
            .Build();

        world->AddComponent<MeshComponent>(id);
        world->AddComponent<ColliderComponent>(id);

        // 形は球か、カプセルっぽく見せるためにSphere
        // 色はType Cに合わせてエメラルドグリーン
        AttachMeshAndCollider(id, world, ShapeType::SPHERE, { 0.0f, 1.0f, 0.8f, 1.0f }, ColliderType::Type_Sphere, 0.8f, 0.0f, 0.0f);

        // 物理挙動 (高速で直進)
        if (!world->GetRegistry()->HasComponent<PhysicsComponent>(id)) {
            float speed = 25.0f; // 敵の弾より速く
            world->AddComponent<PhysicsComponent>(id, PhysicsComponent{
                .velocity = { dir.x * speed, dir.y * speed, dir.z * speed },
                .useGravity = false
                });
        }
        DebugLog("Player Shot Fired! ID: %d", id);
    }
    // ★追加: ヒットエフェクト生成 (火花を散らす)
    inline void CreateHitEffect(World* world, DirectX::XMFLOAT3 pos, int count, DirectX::XMFLOAT4 color) {
        for (int i = 0; i < count; ++i) {
            // ランダムな速度を作る (-5.0 ~ +5.0)
            float vx = (rand() % 100 - 50) / 10.0f;
            float vy = (rand() % 100) / 10.0f + 2.0f; // 上方向に跳ねさせる
            float vz = (rand() % 100 - 50) / 10.0f;

            EntityID id = world->CreateEntity()
                .AddComponent<TransformComponent>(TransformComponent{
                    .position = pos,
                    .scale = {0.2f, 0.2f, 0.2f} // 小さく
                    })
                .AddComponent<ParticleComponent>(ParticleComponent{
                    .lifeTime = 0.5f + (rand() % 10) / 20.0f, // 0.5~1.0秒で消える
                    .velocity = {vx, vy, vz},
                    .useGravity = true,  // 重力で落ちる
                    .scaleSpeed = -0.5f  // 徐々に小さくなる
                    })
                .Build();

            world->AddComponent<MeshComponent>(id);
            // 形は小さめのキューブ
            AttachMeshAndCollider(id, world, ShapeType::CUBE, color, ColliderType::Type_None, 0.0f, 0.0f, 0.0f);
        }
		DebugLog("Created Hit Effect at (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
    }
}