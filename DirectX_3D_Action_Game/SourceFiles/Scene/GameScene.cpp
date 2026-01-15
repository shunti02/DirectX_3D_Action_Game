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
#include <cstdlib>

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
    pUISystem = pWorld->AddSystem<UISystem>(); //ポインタを保存
    pUISystem->Init(pWorld.get());

    // ---------------------------------------------------------
    // 2. ステージ情報の取得
    // ---------------------------------------------------------
    int currentStage = Game::GetInstance()->GetCurrentStage();
    int phase = Game::GetInstance()->GetCurrentPhase();
    int savedHP = Game::GetInstance()->GetSavedHP(); // 前のフェーズからのHP

    AppLog::AddLog("--- STAGE %d : PHASE %d START ---", currentStage, phase);

    DirectX::XMFLOAT3 startPos = { 0.0f, 1.0f, 0.0f };
    // ---------------------------------------------------------
    // 3. エンティティ配置 (EntityFactory利用)
    // ---------------------------------------------------------

   // ---------------------------------------------------------
    // プレイヤー生成 (宇宙仕様)
    // ---------------------------------------------------------
    // ユーザーが選択したタイプを取得
    PlayerType selectedType = Game::GetInstance()->GetPlayerType();

    DirectX::XMFLOAT4 bodyColor;
    float speed = 6.0f;
    float scale = 0.5f;
    int maxHp = 200;      // デフォルトHP
    int attackPower = 15; // デフォルト攻撃力
    // タイプごとのパラメータ設定
    switch (selectedType) {
    case PlayerType::AssaultStriker: // 青：バランス
        bodyColor = { 0.0f, 0.5f, 1.0f, 1.0f }; // Cyan系
        speed = 6.0f;
        maxHp = 200;
        attackPower = 15;
        break;
    case PlayerType::BusterGuard:    // 黄：パワー＆鈍足
        bodyColor = { 1.0f, 0.8f, 0.0f, 1.0f }; // Gold
        speed = 4.0f;
        scale = 0.7f; // 少し大きい
        maxHp = 350;         // かなり硬い
        attackPower = 25;    // 一撃が重い
        break;
    case PlayerType::PlasmaSniper:   // 緑：遠距離＆高速
        bodyColor = { 0.0f, 1.0f, 0.5f, 1.0f }; // Green
        speed = 7.5f;
        maxHp = 120;         // 脆い
        attackPower = 12;    // 手数で勝負
        break;
    }

    // エンティティ生成
    EntityID playerID = EntityFactory::CreateEntity(pWorld.get(), {
        .type = "Player",
        .position = startPos,
        .scale = { scale, scale, scale },
        .color = bodyColor
        });

    // コンポーネント詳細設定
    auto& playerComp = pWorld->GetComponent<PlayerComponent>(playerID);
    playerComp.type = selectedType;
    playerComp.moveSpeed = speed;
    playerComp.isActive = true;

    if (pWorld->GetRegistry()->HasComponent<StatusComponent>(playerID)) {
        auto& status = pWorld->GetRegistry()->GetComponent<StatusComponent>(playerID);
        status.maxHp = maxHp;
        status.attackPower = attackPower;
        status.hp = maxHp; // まずは全快

        // ★HP引き継ぎ処理
        if (savedHP > 0) {
            status.hp = savedHP;
            // 最大値を超えないようにキャップ
            if (status.hp > status.maxHp) status.hp = status.maxHp;
            AppLog::AddLog("HP Carried Over: %d / %d", status.hp, status.maxHp);
        }
    }

    // --- カメラ生成 (追従対象をセット) ---
    EntityID cameraID = EntityFactory::CreateEntity(pWorld.get(), {
        .type = "Camera",
        .position = { 0.0f, 0.0f, 0.0f }
        });
    auto& camComp = pWorld->GetComponent<CameraComponent>(cameraID);
    camComp.targetEntityID = playerID;

   


    // ---------------------------------------------------------
    // 4. 敵の生成ロジック (フェーズ制)
    // ---------------------------------------------------------
    if (phase == 3) {
        // === フェーズ3: BOSS BATTLE ===
        AppLog::AddLog("WARNING: BOSS BATTLE!");

        if (currentStage == 5) {
            // [STAGE 5] ラスボス (超巨大)
            EntityID bossID = EntityFactory::CreateEntity(pWorld.get(), {
                .type = "Enemy2", // 見た目はEnemy2だが...
                .position = { 0.0f, 0.0f, 25.0f },
                .scale = { 4.0f, 4.0f, 4.0f }, // 4倍サイズ
                .color = { 1.0f, 0.0f, 0.0f, 1.0f } // 真っ赤
                });
            // ステータス強化
            if (pWorld->GetRegistry()->HasComponent<StatusComponent>(bossID)) {
                auto& st = pWorld->GetRegistry()->GetComponent<StatusComponent>(bossID);
                st.maxHp = 2000;     // 超高耐久
                st.hp = 2000;
                st.attackPower = 40; // 即死級
            }
        }
        else {
            // [STAGE 1-4] 中ボス (やや大きい)
            EntityID bossID = EntityFactory::CreateEntity(pWorld.get(), {
                .type = "Enemy2",
                .position = { 0.0f, 0.0f, 20.0f },
                .scale = { 2.5f, 2.5f, 2.5f },
                .color = { 1.0f, 0.5f, 0.0f, 1.0f } // オレンジ
                });
            if (pWorld->GetRegistry()->HasComponent<StatusComponent>(bossID)) {
                auto& st = pWorld->GetRegistry()->GetComponent<StatusComponent>(bossID);
                st.maxHp = 150 + (currentStage * 80); // 150 -> 230 -> 310...
                st.hp = st.maxHp;
                st.attackPower = 15 + (currentStage * 5);
            }
        }
    }
    else {
        // === フェーズ1 & 2: 通常ウェーブ ===
        // 敵の数 = ステージ数 + フェーズ数 + 2 (徐々に増える)
        int enemyCount = currentStage + phase + 2;

        for (int i = 0; i < enemyCount; ++i) {
            float x = (rand() % 40) - 20.0f; // 広範囲
            float z = 10.0f + (rand() % 30);

            // 4体に1体はエリート(Enemy2)
            bool isElite = (i % 4 == 0);
            std::string type = isElite ? "Enemy2" : "Enemy";
            float eScale = isElite ? 1.5f : 1.0f;
            DirectX::XMFLOAT4 eColor = isElite ? DirectX::XMFLOAT4(1.0f, 0.2f, 0.2f, 1.0f) : Colors::Red;

            EntityID eid = EntityFactory::CreateEntity(pWorld.get(), {
                .type = type,
                .position = { x, 0.0f, z },
                .scale = { eScale, eScale, eScale },
                .color = eColor
                });

            // 雑魚ステータス調整
            if (pWorld->GetRegistry()->HasComponent<StatusComponent>(eid)) {
                auto& st = pWorld->GetRegistry()->GetComponent<StatusComponent>(eid);
                if (isElite) {
                    st.maxHp = 80 + (currentStage * 20); // エリートは硬い
                    st.attackPower = 10 + currentStage;
                }
                else {
                    st.maxHp = 30 + (currentStage * 10); // 雑魚は倒しやすい
                    st.attackPower = 5 + currentStage;
                }
                st.hp = st.maxHp;
            }
        }
    }

    // ---------------------------------------------------------
    // 回復スポット生成 (エネルギータンク方式)
    // ---------------------------------------------------------
    // 毎フェーズ生成
    EntityID spot = EntityFactory::CreateEntity(pWorld.get(), {
        .type = "HealSpot",
        .position = startPos, // ★同じ場所！
        .rotation = { 0.0f, 0.0f, 0.0f }, // X, Z軸を45度傾けるDirectX::XM_PIDIV4
        .scale = { 0.5f, 0.5f, 0.5f }, // 少し小さめに
        .color = { 0.0f, 1.0f, 0.5f, 0.8f } // 半透明の緑（クリスタル感）
        });

    if (!pWorld->GetRegistry()->HasComponent<RecoverySphereComponent>(spot)) {
        // ★修正: 構造体のメンバに合わせて初期化
        // (radius, healAmount, isActive, capacity, rotationAngle)
        pWorld->GetRegistry()->AddComponent<RecoverySphereComponent>(spot, RecoverySphereComponent{
            .radius = 3.0f,
            .healAmount = 1,   // 使わないが念のため
            .isActive = true,
            .capacity = 300,   // ★容量
            .rotationAngle = 0.0f
            });
    }

    // ---------------------------------------------------------
    // 6. dd★ステージごとの地形生成
    // ---------------------------------------------------------
    // 壁作成用のヘルパーラムダ関数
    auto CreateWall = [&](float x, float z, float w, float d, float h = 2.0f) {
        EntityFactory::CreateEntity(pWorld.get(), {
            .type = "Wall", // 名前は適当でOK(Colliderがあれば壁になる)
            .position = { x, h / 2.0f, z },
            .scale = { w, h, d },
            .color = { 0.5f, 0.5f, 0.5f, 1.0f } // グレー
            });
        };

    // 床 (共通)
    EntityFactory::CreateEntity(pWorld.get(), { .type = "Ground", .position = {0,-1,0}, .scale = {40,1,40} });

    // 外壁 (共通・少し広めに)
    CreateWall(20.0f, 0.0f, 1.0f, 40.0f, 5.0f); // 右
    CreateWall(-20.0f, 0.0f, 1.0f, 40.0f, 5.0f); // 左
    CreateWall(0.0f, 20.0f, 40.0f, 1.0f, 5.0f); // 奥
    CreateWall(0.0f, -20.0f, 40.0f, 1.0f, 5.0f); // 手前

    // ステージ別オブジェクト配置
    switch (currentStage) {
    case 1:
        // [STAGE 1] 広場 (障害物なし)
        // 練習用なので何もしない
        break;

    case 2:
        // [STAGE 2] 遺跡の柱 (4本の柱)
        CreateWall(8.0f, 8.0f, 2.0f, 2.0f, 4.0f);
        CreateWall(-8.0f, 8.0f, 2.0f, 2.0f, 4.0f);
        CreateWall(8.0f, -8.0f, 2.0f, 2.0f, 4.0f);
        CreateWall(-8.0f, -8.0f, 2.0f, 2.0f, 4.0f);
        break;

    case 3:
        // [STAGE 3] 十字路 (中央に巨大なクロス壁)
        CreateWall(0.0f, 0.0f, 20.0f, 2.0f, 3.0f); // 横長の壁
        CreateWall(0.0f, 0.0f, 2.0f, 20.0f, 3.0f); // 縦長の壁
        // ※中央が塞がると移動しにくいので、少し隙間を空けても良いですが、
        // 今回は「壁越しに撃つ・隠れる」遊びにするため十字にします。
        break;

    case 4:
        // [STAGE 4] 市街地・バリケード (ランダムな遮蔽物)
        CreateWall(5.0f, 5.0f, 4.0f, 1.0f, 1.5f); // 低い壁
        CreateWall(-5.0f, 10.0f, 1.0f, 4.0f, 1.5f);
        CreateWall(10.0f, -5.0f, 1.0f, 4.0f, 1.5f);
        CreateWall(-8.0f, -8.0f, 4.0f, 1.0f, 1.5f);
        CreateWall(0.0f, 15.0f, 8.0f, 1.0f, 2.0f); // 奥の壁
        break;

    case 5:
        // [STAGE 5] ラスボスアリーナ
        // 障害物は少ないが、雰囲気を変えるために柱を遠くに置く
        CreateWall(15.0f, 15.0f, 1.0f, 1.0f, 10.0f);
        CreateWall(-15.0f, 15.0f, 1.0f, 1.0f, 10.0f);
        CreateWall(15.0f, -15.0f, 1.0f, 1.0f, 10.0f);
        CreateWall(-15.0f, -15.0f, 1.0f, 1.0f, 10.0f);
        break;
    }

    // ---------------------------------------------------------
    // SkyBox初期化
    // ---------------------------------------------------------
    pSkyBox = std::make_unique<SkyBox>();
    if (!pSkyBox->Initialize(Game::GetInstance()->GetGraphics())) {
        AppLog::AddLog("Error: SkyBox Initialize Failed."); // ログ出力
        pSkyBox.reset(); //失敗したらポインタを空にする！
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
    int currentPlayerHP = 0; // 保存用HP

    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (registry->HasComponent<StatusComponent>(id)) {
            auto& status = registry->GetComponent<StatusComponent>(id);
            if (status.hp <= 0) continue; // 死んでたら無視

            if (registry->HasComponent<PlayerComponent>(id)) {
                alivePlayers++;
                // 生きているプレイヤーのHPを記録
                currentPlayerHP = status.hp;
            }
            else if (registry->HasComponent<EnemyComponent>(id)) {
                remainingEnemies++;
            }
        }
    }

    // --- 敗北 (全滅) ---
    if (alivePlayers == 0) {
        ResultScene::isClear = false;
        // 敗北時はリザルトへ (ステージ進行はリセットされる前提)
        Game::GetInstance()->GetSceneManager()->ChangeScene<ResultScene>();
        return;
    }

    // --- 勝利 (敵全滅) ---
    if (remainingEnemies == 0) {
        int stage = Game::GetInstance()->GetCurrentStage();
        int phase = Game::GetInstance()->GetCurrentPhase();

        if (phase < 3) {
            // --- 次のフェーズへ (HP引き継ぎ) ---
            Game::GetInstance()->NextPhase();
            Game::GetInstance()->SetSavedHP(currentPlayerHP); // ★現在のHPを保存

            // シーンをリロードして次のフェーズを開始
            Game::GetInstance()->GetSceneManager()->ChangeScene<GameScene>();
        }
        else {
            // --- ステージクリア (フェーズ3撃破) ---
            if (stage < 5) {
                // 次のステージへ進む
                Game::GetInstance()->SetCurrentStage(stage + 1);
                Game::GetInstance()->UnlockNextStage();
                Game::GetInstance()->SetCurrentPhase(1); // フェーズ1に戻す
                Game::GetInstance()->SetSavedHP(-1);     // ステージクリア時は全回復 (-1)

                Game::GetInstance()->GetSceneManager()->ChangeScene<GameScene>();
            }
            else {
                // 全クリ！ (ステージ5クリア)
                ResultScene::isClear = true;
                Game::GetInstance()->GetSceneManager()->ChangeScene<ResultScene>();
            }
        }
    }
}