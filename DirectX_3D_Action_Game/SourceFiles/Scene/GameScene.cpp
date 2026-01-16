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
#include "ECS/Systems/ParticleSystem.h"

// システム
#include "ECS/Systems/RenderSystem.h"
#include "ECS/Systems/CameraSystem.h"
#include "ECS/Systems/PlayerSystem.h"
#include "ECS/Systems/PlayerAnimationSystem.h"
#include "ECS/Systems/EnemySystem.h"
#include "ECS/Systems/EnemyAnimationSystem.h"
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
    pWorld->AddSystem<ParticleSystem>()->Init(pWorld.get());

    m_pEnemyAnimSystem = pWorld->AddSystem<EnemyAnimationSystem>();
    m_pEnemyAnimSystem->Init(pWorld.get());

    m_pAnimSystem = pWorld->AddSystem<PlayerAnimationSystem>();
    m_pAnimSystem->Init(pWorld.get());

    m_pCameraSystem = pWorld->AddSystem<CameraSystem>();
    m_pCameraSystem->Init(pWorld.get());

    m_pRenderSystem = pWorld->AddSystem<RenderSystem>();
    m_pRenderSystem->Init(pWorld.get());

    pUISystem = pWorld->AddSystem<UISystem>(); //ポインタを保存
    pUISystem->Init(pWorld.get());

    // ---------------------------------------------------------
    // ★追加: タイマーリセット
    // ---------------------------------------------------------
    m_isSceneChanging = false;
    m_startTimer = 0.0f;
    // ---------------------------------------------------------
    // 2. ステージ情報の取得
    // ---------------------------------------------------------
    int currentStage = Game::GetInstance()->GetCurrentStage();
    int phase = Game::GetInstance()->GetCurrentPhase();
    int savedHP = Game::GetInstance()->GetSavedHP(); // 前のフェーズからのHP

    AppLog::AddLog("--- STAGE %d : PHASE %d START ---", currentStage, phase);

    DirectX::XMFLOAT3 startPos = { 0.0f, 1.0f, 0.0f };

    // ステージごとのプレイヤー開始位置調整
    switch (currentStage) {
    case 1:
        startPos = { 0.0f, 1.0f, -10.0f }; // 手前側からスタート
        break;
    case 2:
        startPos = { 0.0f, 1.0f, -10.0f };   // 中央（柱の間）は安全
        break;
    case 3:
        startPos = { -8.0f, 1.0f, -8.0f }; // 左下の区画からスタート（中央は壁）
        break;
    case 4:
        startPos = { 0.0f, 1.0f, -12.0f }; // 手前の安全地帯
        break;
    case 5:
        startPos = { 0.0f, 1.0f, -18.0f }; // リングの端（ボスと対面）
        break;
    }
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
        attackPower = 150;
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
    // 4. 敵の生成ロジック (難易度調整版)
    // ---------------------------------------------------------
    if (phase == 3) {
        // === フェーズ3: BOSS BATTLE ===
        AppLog::AddLog("WARNING: BOSS BATTLE!");

        // ボスの出現位置
        DirectX::XMFLOAT3 bossPos = { 0.0f, 0.0f, 20.0f };
        if (currentStage == 3) bossPos = { 8.0f, 0.0f, 8.0f }; // 右上
        if (currentStage == 5) bossPos = { 0.0f, 0.0f, 0.0f }; // 中央

        if (currentStage == 5) {
            // [STAGE 5] ラスボス (Bossタイプ)
            EntityFactory::CreateEntity(pWorld.get(), {
                .type = "Boss", // ★ラスボス専用タイプを使用
                .position = bossPos,
                .color = { 1.0f, 0.0f, 0.0f, 1.0f }
                });
        }
        else {
            // [STAGE 1-4] 中ボス (Enemy2タイプを強化)
            // ステージが進むほどHPと攻撃力が上がる
            EntityID bossID = EntityFactory::CreateEntity(pWorld.get(), {
                .type = "Enemy2", // 中ボスタイプ
                .position = bossPos,
                .scale = { 2.5f, 2.5f, 2.5f },
                .color = { 1.0f, 0.5f, 0.0f, 1.0f } // オレンジ
                });

            // ステータス強化
            if (pWorld->GetRegistry()->HasComponent<StatusComponent>(bossID)) {
                auto& st = pWorld->GetRegistry()->GetComponent<StatusComponent>(bossID);
                st.maxHp = 150 + (currentStage * 100); // 250 -> 350 -> 450 -> 550
                st.hp = st.maxHp;
                st.attackPower = 15 + (currentStage * 5);
            }
        }
    }
    else {
        // === フェーズ1 & 2: 通常ウェーブ ===

        // 敵の数 = (ステージ * 2) + (フェーズ * 2) + 1
        // 例: St1-Ph1=5体, St1-Ph2=7体, St5-Ph1=13体
        int enemyCount = (currentStage * 2) + (phase * 2) + 1;

        for (int i = 0; i < enemyCount; ++i) {
            float x = 0.0f, z = 0.0f;

            // 出現位置 (ステージ別)
            switch (currentStage) {
            case 1: x = (rand() % 30) - 15.0f; z = 5.0f + (rand() % 15); break;
            case 2: x = (rand() % 24) - 12.0f; z = (rand() % 24) - 12.0f; if (abs(x) < 5 && abs(z) < 5) x += 10; break;
            case 3:
                if (rand() % 3 == 0) { x = -8; z = 8; }
                else if (rand() % 2 == 0) { x = 8; z = 8; }
                else { x = 8; z = -8; }
                x += (rand() % 6) - 3.0f; z += (rand() % 6) - 3.0f;
                break;
            case 4: x = (rand() % 36) - 18.0f; z = (rand() % 36) - 18.0f; break;
            case 5:
                float angle = (rand() % 360) * 3.14f / 180.0f;
                float r = 20.0f;
                x = cosf(angle) * r; z = sinf(angle) * r;
                break;
            }

            // 敵の種類決定 (ランダム抽選)
            std::string type = "Enemy"; // デフォルト: 雑魚(近接)
            float scale = 1.0f;
            DirectX::XMFLOAT4 color = { 1.0f, 0.2f, 0.2f, 1.0f }; // 赤

            int rnd = rand() % 100; // 0~99

            if (currentStage == 1) {
                // STAGE 1: ほぼ雑魚のみ
                if (rnd < 10) type = "EnemyRanged"; // 10% 遠距離
            }
            else if (currentStage == 2) {
                // STAGE 2: 遠距離が増える
                if (rnd < 30) type = "EnemyRanged"; // 30% 遠距離
                else if (rnd < 35) type = "Enemy2"; // 5% 中ボス(小型)
            }
            else if (currentStage >= 3) {
                // STAGE 3以降: 激戦
                if (rnd < 40) type = "EnemyRanged"; // 40% 遠距離
                else if (rnd < 55) type = "Enemy2"; // 15% 中ボス
            }

            // 中ボスが雑魚として出る場合の調整
            if (type == "Enemy2") {
                scale = 1.5f; // 本物の中ボスよりは小さく
                color = { 1.0f, 0.8f, 0.2f, 1.0f }; // 黄色
            }
            else if (type == "EnemyRanged") {
                color = { 0.8f, 0.2f, 1.0f, 1.0f }; // 紫
            }

            // 生成実行
            EntityID eid = EntityFactory::CreateEntity(pWorld.get(), {
                .type = type,
                .position = { x, 0.0f, z },
                .scale = { scale, scale, scale },
                .color = color
                });

            // ステータス調整 (ステージが進むと少し強くなる)
            if (pWorld->GetRegistry()->HasComponent<StatusComponent>(eid)) {
                auto& st = pWorld->GetRegistry()->GetComponent<StatusComponent>(eid);
                // HP増加: 基礎値 + (ステージ * 5)
                st.maxHp += (currentStage * 5);
                st.hp = st.maxHp;
                // 攻撃力増加
                st.attackPower += currentStage;
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
    // 6. ステージごとの地形生成 (完成版)
    // ---------------------------------------------------------

    // 壁作成ラムダ (色指定を追加)
    auto CreateWall = [&](float x, float z, float w, float d, float h, DirectX::XMFLOAT4 color) {
        EntityFactory::CreateEntity(pWorld.get(), {
            .type = "Wall",
            .position = { x, h / 2.0f, z },
            .scale = { w, h, d },
            .color = color
            });
        };

    // --- カラーパレット定義 ---
    DirectX::XMFLOAT4 colConcrete = { 0.5f, 0.5f, 0.5f, 1.0f }; // コンクリートグレー
    DirectX::XMFLOAT4 colRuins = { 0.8f, 0.7f, 0.5f, 1.0f }; // 砂岩色
    DirectX::XMFLOAT4 colCyber = { 0.0f, 0.1f, 0.3f, 1.0f }; // ダークブルー
    DirectX::XMFLOAT4 colNeon = { 0.0f, 0.8f, 1.0f, 1.0f }; // ネオンシアン
    DirectX::XMFLOAT4 colMagma = { 0.4f, 0.1f, 0.1f, 1.0f }; // 暗い赤

    // --- 地面と外壁 (ステージによって色を変える) ---
    DirectX::XMFLOAT4 groundColor = colConcrete;
    DirectX::XMFLOAT4 wallColor = colConcrete;

    if (currentStage == 2) { groundColor = { 0.6f, 0.5f, 0.3f, 1.0f }; wallColor = colRuins; }
    if (currentStage == 3) { groundColor = { 0.1f, 0.1f, 0.1f, 1.0f }; wallColor = colCyber; }
    if (currentStage == 5) { groundColor = { 0.2f, 0.0f, 0.0f, 1.0f }; wallColor = colMagma; }

    // 床
    EntityFactory::CreateEntity(pWorld.get(), { .type = "Ground", .position = {0,-1,0}, .scale = {60, 1, 60}, .color = groundColor });

    // 外壁 (エリア制限)
    CreateWall(30.0f, 0.0f, 1.0f, 60.0f, 10.0f, wallColor); // 右
    CreateWall(-30.0f, 0.0f, 1.0f, 60.0f, 10.0f, wallColor); // 左
    CreateWall(0.0f, 30.0f, 60.0f, 1.0f, 10.0f, wallColor); // 奥
    CreateWall(0.0f, -30.0f, 60.0f, 1.0f, 10.0f, wallColor); // 手前

    // --- オブジェクト配置 ---
    switch (currentStage) {
    case 1:
        // [STAGE 1] 演習場 (Training Field)
        // 明るいグレーの低い遮蔽物が点在。基本操作の練習用。
    {
        DirectX::XMFLOAT4 c = { 0.7f, 0.7f, 0.7f, 1.0f };
        // 手前の左右に低いカバー
        CreateWall(10.0f, -10.0f, 4.0f, 1.0f, 1.5f, c);
        CreateWall(-10.0f, -10.0f, 4.0f, 1.0f, 1.5f, c);
        // 奥に少し高い壁
        CreateWall(0.0f, 15.0f, 8.0f, 2.0f, 3.0f, c);
    }
    break;

    case 2:
        // [STAGE 2] 古代遺跡 (Ruins)
        // 砂漠のような色合い。高く太い柱があり、大きく回る動きが必要。
    {
        // 四隅に巨大な柱
        CreateWall(12.0f, 12.0f, 3.0f, 3.0f, 8.0f, colRuins);
        CreateWall(-12.0f, 12.0f, 3.0f, 3.0f, 8.0f, colRuins);
        CreateWall(12.0f, -12.0f, 3.0f, 3.0f, 8.0f, colRuins);
        CreateWall(-12.0f, -12.0f, 3.0f, 3.0f, 8.0f, colRuins);

        // 中央付近に倒れた柱（低い障害物）
        CreateWall(0.0f, 0.0f, 6.0f, 2.0f, 1.0f, colRuins);
    }
    break;

    case 3:
        // [STAGE 3] サイバーベース (Cyber Base)
        // 暗い青色の迷路。視界が悪く、曲がり角での遭遇戦。
    {
        // 中央の十字路を作る壁
        // 横壁
        CreateWall(15.0f, 0.0f, 20.0f, 1.0f, 4.0f, colCyber);
        CreateWall(-15.0f, 0.0f, 20.0f, 1.0f, 4.0f, colCyber);
        // 縦壁
        CreateWall(0.0f, 15.0f, 1.0f, 20.0f, 4.0f, colCyber);
        CreateWall(0.0f, -15.0f, 1.0f, 20.0f, 4.0f, colCyber);

        // 角のネオンポール（目印）
        CreateWall(5.0f, 5.0f, 0.5f, 0.5f, 6.0f, colNeon);
        CreateWall(-5.0f, 5.0f, 0.5f, 0.5f, 6.0f, colNeon);
        CreateWall(5.0f, -5.0f, 0.5f, 0.5f, 6.0f, colNeon);
        CreateWall(-5.0f, -5.0f, 0.5f, 0.5f, 6.0f, colNeon);
    }
    break;

    case 4:
        // [STAGE 4] 市街地・廃墟 (Ruined City)
        // ランダムで複雑な地形。高さもバラバラで、立体的な射撃戦。
    {
        DirectX::XMFLOAT4 debris = { 0.4f, 0.4f, 0.45f, 1.0f };
        // バリケード
        CreateWall(5.0f, 5.0f, 5.0f, 1.0f, 1.5f, debris);
        CreateWall(-8.0f, 8.0f, 1.0f, 5.0f, 2.5f, debris);
        CreateWall(10.0f, -10.0f, 4.0f, 4.0f, 1.0f, debris); // 台座

        // L字型の壁
        CreateWall(-15.0f, -15.0f, 8.0f, 1.0f, 3.0f, debris);
        CreateWall(-19.0f, -11.0f, 1.0f, 8.0f, 3.0f, debris);

        // 浮遊ブロック（乗れないが遮蔽になる）
        CreateWall(0.0f, 10.0f, 3.0f, 3.0f, 3.0f, colNeon); // 中央上空のオブジェ
    }
    break;

    case 5:
        // [STAGE 5] ラスボスアリーナ (Crimson Core)
        // 赤く染まった広い空間。障害物は少なく、ボスとのタイマン仕様。
    {
        // 雰囲気を出すための巨大な柱（エリア端）
        // プレイヤーの邪魔になりすぎないように外側に配置
        CreateWall(20.0f, 20.0f, 2.0f, 2.0f, 15.0f, colMagma);
        CreateWall(-20.0f, 20.0f, 2.0f, 2.0f, 15.0f, colMagma);
        CreateWall(20.0f, -20.0f, 2.0f, 2.0f, 15.0f, colMagma);
        CreateWall(-20.0f, -20.0f, 2.0f, 2.0f, 15.0f, colMagma);

        // 中央は何もないが、床の色を変えて「リング」感を出す（演出）
        // ※EntityFactoryで床をもう一枚薄く重ねるなどしても良いが、ここではシンプルに。
    }
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

    // ---------------------------------------------------------
     // ★修正: 開始演出の制御 (フェーズによって時間を変える)
     // ---------------------------------------------------------
    m_startTimer += dt;

    int currentPhase = Game::GetInstance()->GetCurrentPhase();

    // フェーズ1なら3.0秒(Ready...Go!)、それ以外(Phase移行)なら2.0秒(PHASE X)待つ
    float waitTime = (currentPhase == 1) ? 3.0f : 2.0f;

    if (m_startTimer < waitTime) {
        // === 準備中 ===
        // ゲームロジックは止める
        if (m_pAnimSystem) m_pAnimSystem->Update(dt);
        if (m_pEnemyAnimSystem) m_pEnemyAnimSystem->Update(dt);
        if (m_pCameraSystem) m_pCameraSystem->Update(dt);
        if (m_pRenderSystem) m_pRenderSystem->Update(dt);
    }
    else {
        // === ゲーム開始 ===
        BaseScene::Update(dt);
        CheckGameCondition();
    }
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

    // ---------------------------------------------------------
     // ★修正: 開始演出の描画 (分岐)
     // ---------------------------------------------------------
    Graphics* g = Game::GetInstance()->GetGraphics();
    if (g) {
        g->BeginDraw2D();

        int currentPhase = Game::GetInstance()->GetCurrentPhase();

        // === [STAGE START] (フェーズ1) の場合 ===
        if (currentPhase == 1) {
            if (m_startTimer < 2.0f) {
                // [0.0 ~ 2.0秒] READY...
                g->DrawString(L"READY...", 500.0f, 300.0f, 80.0f, 0xFFFFFFFF);
            }
            else if (m_startTimer < 3.0f) {
                // [2.0 ~ 3.0秒] GO !!
                g->DrawString(L"GO !!", 550.0f, 300.0f, 100.0f, 0xFFFF0000);
            }
            else if (m_startTimer < 4.0f) {
                // [3.0 ~ 4.0秒] GOのフェードアウト
                float alpha = 1.0f - (m_startTimer - 3.0f);
                uint32_t a = static_cast<uint32_t>(alpha * 255.0f);
                uint32_t col = (a << 24) | 0xFF0000; // 赤
                g->DrawString(L"GO !!", 550.0f, 300.0f, 100.0f + (m_startTimer - 3.0f) * 50.0f, col);
            }
        }
        // === [PHASE TRANSITION] (フェーズ2以降) の場合 ===
        else {
            // 合計2.0秒のアニメーション
            // 0.0 ~ 0.5 : フェードイン
            // 0.5 ~ 1.5 : 表示
            // 1.5 ~ 2.0 : フェードアウト

            float alpha = 0.0f;
            float scale = 1.0f;

            if (m_startTimer < 0.5f) {
                // フェードイン (ふわっと出る)
                alpha = m_startTimer / 0.5f;
                scale = 0.5f + (alpha * 0.5f); // 少し拡大しながら
            }
            else if (m_startTimer < 1.5f) {
                // 表示維持
                alpha = 1.0f;
                scale = 1.0f;
            }
            else if (m_startTimer < 2.5f) { // 少し余韻を残す
                // フェードアウト
                float t = (m_startTimer - 1.5f);
                alpha = 1.0f - t;
                scale = 1.0f + (t * 0.2f); // さらに拡大しながら消える
            }

            if (alpha > 0.0f) {
                uint32_t a = static_cast<uint32_t>(alpha * 255.0f);
                uint32_t col = (a << 24) | 0xFFFFFF; // 白

                std::wstring text = L"PHASE " + std::to_wstring(currentPhase);

                // 中央に表示 (文字サイズ80)
                float size = 80.0f * scale;
                // 簡易的な中央寄せ計算 (文字数 * サイズ * 係数)
                float x = (float)Config::SCREEN_WIDTH / 2.0f - (text.length() * size * 0.3f);
                float y = (float)Config::SCREEN_HEIGHT / 2.0f - (size * 0.5f);

                g->DrawString(text.c_str(), x, y, size, col);
            }
        }

        g->EndDraw2D();
    }
}

void GameScene::CheckGameCondition() {
    if (m_isSceneChanging) return;
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
        m_isSceneChanging = true;
        ResultScene::isClear = false;
        // 敗北時はリザルトへ (ステージ進行はリセットされる前提)
        Game::GetInstance()->GetSceneManager()->ChangeScene<ResultScene>();
        return;
    }

    // --- 勝利 (敵全滅) ---
    if (remainingEnemies == 0) {
        m_isSceneChanging = true;
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
            // --- ★修正: ステージクリア処理 ---

            // 1. 次のステージを解放
            Game::GetInstance()->UnlockNextStage();

            // 2. セーブを実行！
            Game::GetInstance()->SaveGame();

            // 3. 常にリザルト画面へ行く (自動進行しない)
            ResultScene::isClear = true;
            Game::GetInstance()->GetSceneManager()->ChangeScene<ResultScene>();
        }
    }
}