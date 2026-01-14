/*===================================================================
// ファイル: GameScene.cpp
// 概要: ゲームプレイ中のシーン（実装）
=====================================================================*/
#include "Scene/GameScene.h"

// 必要なヘッダ群を一括インクルード
#include "App/Game.h"
#include "Game/EntityFactory.h"
#include "Scene/ResultScene.h"

// コンポーネント
#include "ECS/Components/StatusComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/EnemyComponent.h"
#include "ECS/Components/CameraComponent.h"
#include "ECS/Components/TransformComponent.h"

// システム
#include "ECS/Systems/RenderSystem.h"
#include "ECS/Systems/CameraSystem.h"
#include "ECS/Systems/PlayerSystem.h"
#include "ECS/Systems/PlayerAnimationSystem.h"
#include "ECS/Systems/EnemySystem.h"
#include "ECS/Systems/PhysicsSystem.h"
#include "ECS/Systems/ActionSystem.h"
#include "ECS/Systems/UISystem.h"

#include <iostream>

void GameScene::Initialize() {
    // ---------------------------------------------------------
    // 1. システム登録
    // ---------------------------------------------------------
    // 登録順序が重要です（Updateは登録順に実行されます）
    pWorld->AddSystem<PlayerSystem>()->Init(pWorld.get());
    pWorld->AddSystem<EnemySystem>()->Init(pWorld.get());
    pWorld->AddSystem<ActionSystem>()->Init(pWorld.get());
    pWorld->AddSystem<PhysicsSystem>()->Init(pWorld.get());
    pWorld->AddSystem<PlayerAnimationSystem>()->Init(pWorld.get());
    pWorld->AddSystem<CameraSystem>()->Init(pWorld.get());
    pWorld->AddSystem<RenderSystem>()->Init(pWorld.get());
    pUISystem = pWorld->AddSystem<UISystem>(); // ★ポインタを保存
    pUISystem->Init(pWorld.get());

    // ---------------------------------------------------------
    // 2. エンティティ配置 (EntityFactory利用)
    // ---------------------------------------------------------

    // --- プレイヤー生成 (2人) ---

    // 1人目: 攻撃担当 (青)
    EntityID player1 = EntityFactory::CreateEntity(pWorld.get(), {
        .type = "Player",
        .position = { 0.0f, 0.0f, 0.0f },
        .scale = { 0.5f, 0.5f, 0.5f },
        .color = Colors::Blue,
        .role = PlayerRole::Attacker
        });
    // 生成後にコンポーネントを操作する場合はIDを使います
    pWorld->GetComponent<PlayerComponent>(player1).isActive = true;

    // 2人目: 回復担当 (緑)
    EntityFactory::CreateEntity(pWorld.get(), {
        .type = "Player",
        .position = { 2.0f, 0.0f, 0.0f },
        .scale = { 0.5f, 0.5f, 0.5f },
        .color = Colors::Green,
        .role = PlayerRole::Healer
        });

    // --- カメラ生成 ---
    EntityID cameraID = EntityFactory::CreateEntity(pWorld.get(), {
        .type = "Camera",
        .position = { 0.0f, 0.0f, 0.0f }
        });
    // カメラの追従設定
    auto& camComp = pWorld->GetComponent<CameraComponent>(cameraID);
    camComp.targetEntityID = player1;
    camComp.distance = 10.0f;
    camComp.height = 4.0f;
    camComp.lookAtOffset = 0.0f;

    // --- 敵生成 ---
    EntityFactory::CreateEntity(pWorld.get(), {
        .type = "Enemy",
        .position = { 0.0f, 0.0f, 20.0f },
        .scale = { 1.0f, 1.0f, 1.0f },
        .color = Colors::Red
        });

    EntityFactory::CreateEntity(pWorld.get(), {
        .type = "Enemy2",
        .position = { 3.0f, 0.0f, 20.0f },
        .scale = { 1.0f, 1.0f, 1.0f },
        .color = Colors::Yellow
        });

    // --- 地面生成 ---
    // 床
    EntityFactory::CreateEntity(pWorld.get(), {
        .type = "Ground",
        .position = { 0.0f, -1.0f, 0.0f },
		.scale = { 30.0f, 1.0f, 30.0f}
        });
    // 壁
    EntityFactory::CreateEntity(pWorld.get(), {
        .type = "Ground2",
        .position = { 14.5f, 0.0f, 0.0f },
        .scale = { 1.0f, 1.0f, 30.0f }
        });
    // 壁2
    EntityFactory::CreateEntity(pWorld.get(), {
        .type = "Ground3",
        .position = { 0.0f, 0.0f, 14.5f },
        .scale = { 30.0f, 1.0f, 1.0f }
        });
    // 壁2
    EntityFactory::CreateEntity(pWorld.get(), {
        .type = "Ground4",
        .position = { -14.5f, 0.0f, 0.0f },
        .scale = { 1.0f, 1.0f, 30.0f }
        });

    // ---------------------------------------------------------
    // ★追加: SkyBox初期化
    // ---------------------------------------------------------
    pSkyBox = std::make_unique<SkyBox>();
    if (!pSkyBox->Initialize(Game::GetInstance()->GetGraphics())) {
        AppLog::AddLog("Error: SkyBox Initialize Failed."); // ログ出力
        pSkyBox.reset(); // ★重要: 失敗したらポインタを空にする！
    }

#ifdef _DEBUG
    std::cout << "GameScene Initialized." << std::endl;
#endif
}

void GameScene::Update(float dt) {
    // 1. 全システムのUpdateを一括実行 (BaseSceneの機能)
    BaseScene::Update(dt);

    // 2. 勝敗判定
    CheckGameCondition();
}

void GameScene::Draw() {
    // 1. 通常のシーン描画 (RenderSystemなどが実行される)
    BaseScene::Draw();

    // 2. SkyBox描画
    // カメラ情報の取得
    auto registry = pWorld->GetRegistry();
    XMMATRIX view = XMMatrixIdentity();
    XMMATRIX proj = XMMatrixIdentity();

    // アクティブなカメラを探す
    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (registry->HasComponent<CameraComponent>(id)) {
            auto& cam = registry->GetComponent<CameraComponent>(id);
            view = cam.view;
            proj = cam.projection;
            break; // 1つ見つければOK
        }
    }

    // 背景描画実行
    if (pSkyBox) {
        pSkyBox->Draw(Game::GetInstance()->GetGraphics(), view, proj);
    }

    Game::GetInstance()->GetGraphics()->GetContext()->OMSetDepthStencilState(nullptr, 0);
    //UI描画
    if (pUISystem) {
        pUISystem->Draw(Game::GetInstance()->GetGraphics());
    }
}

void GameScene::CheckGameCondition() {
    auto registry = pWorld->GetRegistry();

    int alivePlayers = 0;
    int remainingEnemies = 0;

    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (registry->HasComponent<StatusComponent>(id)) {
            auto& status = registry->GetComponent<StatusComponent>(id);

            // プレイヤーカウント
            if (registry->HasComponent<PlayerComponent>(id)) {
                if (status.hp > 0) alivePlayers++;
            }
            // 敵カウント (プレイヤー以外でStatusを持つもの)
            else if (!registry->HasComponent<PlayerComponent>(id)) {
                // 地面などがStatusを持たない前提
                if (status.hp > 0) remainingEnemies++;
            }
        }
    }

    // --- 敗北判定 (プレイヤー全滅) ---
    if (alivePlayers == 0) {
        ResultScene::isClear = false;
        Game::GetInstance()->GetSceneManager()->ChangeScene<ResultScene>();
        return;
    }

    // --- 勝利判定 (敵全滅) ---
    if (remainingEnemies == 0) {
        ResultScene::isClear = true;
        Game::GetInstance()->GetSceneManager()->ChangeScene<ResultScene>();
        return;
    }
}