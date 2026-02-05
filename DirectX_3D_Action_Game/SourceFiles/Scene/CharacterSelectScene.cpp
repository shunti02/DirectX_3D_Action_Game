/*===================================================================
// ファイル: CharacterSelectScene.cpp
// 概要: キャラクター選択画面 (Direct2D描画マウス/キーボード対応版)
=====================================================================*/
#include "Scene/CharacterSelectScene.h"
#include "Scene/StageSelectScene.h"
#include "Scene/GameScene.h" 
#include "Engine/Input.h"
#include "App/Main.h"
#include "Game/EntityFactory.h" 
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/PlayerPartComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Systems/PlayerAnimationSystem.h"
#include "ECS/Systems/RenderSystem.h"
// ★演出用変数 (staticで簡易定義)
static float cs_blinkTimer = 0.0f;
// コンストラクタ
CharacterSelectScene::CharacterSelectScene(SceneManager* manager)
    : BaseScene(manager)
{
}

void CharacterSelectScene::Initialize() {
    // 現在の設定を読み込み
    PlayerType current = Game::GetInstance()->GetPlayerType();
    m_currentSelectIndex = (int)current;

    // --- 背景演出用の初期化 ---
    pSkyBox = std::make_unique<SkyBox>();
    if (!pSkyBox->Initialize(Game::GetInstance()->GetGraphics())) {
        pSkyBox.reset();
    }

    // ---------------------------------------------------------
    // ★追加: システムの登録
    // ---------------------------------------------------------
    // モデルを表示するには RenderSystem が、パーツを動かすには AnimSystem が必要です
    // ※BaseSceneですでにWorldは作られています

    // アニメーションシステム登録
    m_pAnimSystem = pWorld->AddSystem<PlayerAnimationSystem>();
    m_pAnimSystem->Init(pWorld.get());

    // 描画システム登録 (GameSceneと同じように)
    auto renderSys = pWorld->AddSystem<RenderSystem>();
    renderSys->Init(pWorld.get());

    // プレビューモデル生成
    UpdatePreviewModel();

    // ---------------------------------------------------------
    // ★追加: 描画用のメインカメラを生成
    // ---------------------------------------------------------
    // これを作らないとRenderSystemはデフォルトの位置(0,0,0)から描画してしまいます
    EntityFactory::CreateEntity(pWorld.get(), {
        .type = "Camera",
        .position = { 0.0f, 0.0f, -10.0f } // 初期位置
        });

    AppLog::AddLog("--- Character Select Scene (Hybrid Input) ---");
}

void CharacterSelectScene::Update(float dt) {
    // タイマー更新
    cs_blinkTimer += dt;
    Input* input = Game::GetInstance()->GetInput();
    bool changed = false;
    bool decided = false;

    // ---------------------------------------------------------
    // 1. マウス操作の判定
    // ---------------------------------------------------------
    DirectX::XMFLOAT2 mousePos = input->GetMousePosition();
    float mx = mousePos.x;
    float my = mousePos.y;

    // 画面中央
    float cX = Config::SCREEN_WIDTH / 2.0f;

    // 左矢印 "<" の判定 (画面左側)
    float leftArrowX = 150.0f;
    bool hoverLeft = (mx >= leftArrowX && mx <= leftArrowX + ARROW_SIZE && my >= ARROW_Y && my <= ARROW_Y + ARROW_SIZE);

    // 右矢印 ">" の判定 (画面右側)
    float rightArrowX = Config::SCREEN_WIDTH - 150.0f - ARROW_SIZE;
    bool hoverRight = (mx >= rightArrowX && mx <= rightArrowX + ARROW_SIZE && my >= ARROW_Y && my <= ARROW_Y + ARROW_SIZE);

    // --- 左へ切り替え ---
    if (input->IsKeyDown('A') || input->IsKeyDown(VK_LEFT) || (hoverLeft && input->IsMouseKeyDown(0))) {
        m_currentSelectIndex--;
        if (m_currentSelectIndex < 0) m_currentSelectIndex = 2;
        changed = true;
    }
    // --- 右へ切り替え ---
    if (input->IsKeyDown('D') || input->IsKeyDown(VK_RIGHT) || (hoverRight && input->IsMouseKeyDown(0))) {
        m_currentSelectIndex++;
        if (m_currentSelectIndex > 2) m_currentSelectIndex = 0;
        changed = true;
    }

    // --- 決定 ---
    // エンターキー、スペースキー、または画面中央(モデル付近)をクリック
    if (input->IsKeyDown(VK_RETURN) || input->IsKeyDown(VK_SPACE)) {
        decided = true;
    }
    // 中央クリックでも決定 (矢印以外をクリックした場合)
    if (input->IsMouseKeyDown(0) && !hoverLeft && !hoverRight) {
        // ボタン連打防止のため少しクールタイムがあっても良いが、今回は即決定
        decided = true;
    }

    // ---------------------------------------------------------
    // 2. 状態更新
    // ---------------------------------------------------------
    if (changed) {
        if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_SWITCH");
        UpdatePreviewModel(); // モデルを作り直す
    }

    // モデルを回す
    if (previewModelID != -1) {
        auto& trans = pWorld->GetComponent<TransformComponent>((EntityID)previewModelID);
        trans.rotation.y += 1.0f * dt;
    }

    // システム更新
    if (m_pAnimSystem) m_pAnimSystem->Update(dt);
    if (pWorld) pWorld->Update(dt);

    // ---------------------------------------------------------
    // カメラの位置・設定を強制的に更新
    // ---------------------------------------------------------
    auto registry = pWorld->GetRegistry();
    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (registry->HasComponent<CameraComponent>(id)) {
            auto& cam = registry->GetComponent<CameraComponent>(id);

            // ここでカメラの位置と注視点を設定（全身が映るように調整）
            // カメラの位置 (少し下げて、後ろへ引く)
            XMVECTOR eye = DirectX::XMVectorSet(0.0f, 1.0f, -7.0f, 0.0f);

            XMVECTOR focus = DirectX::XMVectorSet(0.0f, -1.5f, 0.0f, 0.0f);

            XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

            // 行列を計算してCameraComponentにセット
            cam.view = DirectX::XMMatrixLookAtLH(eye, focus, up);

            cam.projection = DirectX::XMMatrixPerspectiveFovLH(
                DirectX::XMConvertToRadians(45.0f),
                Config::SCREEN_WIDTH / (float)Config::SCREEN_HEIGHT,
                0.1f, 1000.0f
            );

            break;
        }
    }

    // ---------------------------------------------------------
    // 3. シーン遷移
    // ---------------------------------------------------------
    if (decided) {
        Game::GetInstance()->SetPlayerType((PlayerType)m_currentSelectIndex);
        Game::GetInstance()->SetCurrentStage(1);
        Game::GetInstance()->GetSceneManager()->ChangeScene<StageSelectScene>();

        if (auto audio = Game::GetInstance()->GetAudio()) {
            audio->Play("SE_JUMP");
        }
    }
}

void CharacterSelectScene::Draw() {
    Graphics* pGraphics = Game::GetInstance()->GetGraphics();

    // 1. 背景描画
    if (pSkyBox) {
        XMVECTOR eye = DirectX::XMVectorSet(0.0f, 1.5f, -9.5f, 0.0f);
        XMVECTOR focus = DirectX::XMVectorSet(0.0f, -0.5f, 0.0f, 0.0f);
        XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        XMMATRIX view = DirectX::XMMatrixLookAtLH(eye, focus, up);
        XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(45.0f),
            Config::SCREEN_WIDTH / (float)Config::SCREEN_HEIGHT, 0.1f, 1000.0f);
        pSkyBox->Draw(pGraphics, view, proj);
    }

    // 2. モデル描画
    BaseScene::Draw();

    // 3. UI描画 (詳細情報)
    if (pGraphics) {
        pGraphics->BeginDraw2D();

        float scW = (float)Config::SCREEN_WIDTH;
        float scH = (float)Config::SCREEN_HEIGHT;

        // ★スキャンライン (画面全体)
        for (float y = 0; y < scH; y += 4.0f) {
            pGraphics->DrawRect(0, y, scW, 1.0f, 0x22000000);
        }

        // --- ヘッダー帯 (グリッチ演出) ---
        float headerY = 50.0f;

        // グリッチ判定
        bool isGlitch = (rand() % 100 < 3); // 3%の確率
        float offX = isGlitch ? (rand() % 6 - 3.0f) : 0.0f;
        float offY = isGlitch ? (rand() % 6 - 3.0f) : 0.0f;
        uint32_t headerCol = isGlitch ? 0xFF00FFFF : 0xFFFFFFFF;

        pGraphics->DrawString(L"SELECT CHARACTER", 100.0f + offX, headerY + offY, 48.0f, headerCol);
        // 残像
        if (!isGlitch) pGraphics->DrawString(L"SELECT CHARACTER", 102.0f, headerY, 48.0f, 0x4400FFFF);

        pGraphics->DrawRect(0, headerY + 60.0f, scW, 2.0f, 0xFF00FFFF); // シアンライン

        // --- 左右の矢印ボタン ---
        float leftArrowX = 150.0f;
        float rightArrowX = scW - 150.0f - ARROW_SIZE;

        // 点滅アニメーション
        float alpha = (sinf(cs_blinkTimer * 5.0f) + 1.0f) * 0.5f;
        uint32_t glowCol = (static_cast<uint32_t>(alpha * 150.0f) << 24) | 0x00FFFF00;

        pGraphics->DrawRectOutline(leftArrowX, ARROW_Y, ARROW_SIZE, ARROW_SIZE, 2.0f, 0xFF00FFFF);
        pGraphics->DrawString(L"<", leftArrowX + 25, ARROW_Y + 10, 60.0f, 0xFFFFFFFF);

        pGraphics->DrawRectOutline(rightArrowX, ARROW_Y, ARROW_SIZE, ARROW_SIZE, 2.0f, 0xFF00FFFF);
        pGraphics->DrawString(L">", rightArrowX + 25, ARROW_Y + 10, 60.0f, 0xFFFFFFFF);

        // --- 下部情報パネル ---
        float panelY = 500.0f;
        float panelH = 220.0f;

        // 背景 (濃い紺色 + 透過)
        pGraphics->FillRect(0, panelY, scW, panelH, 0xCC001133);
        pGraphics->DrawRectOutline(0, panelY, scW, panelH, 2.0f, 0xFF00FFFF);

        // キャラクター名
        const wchar_t* names[] = { L"ASSAULT STRIKER", L"BUSTER GUARD", L"PLASMA SNIPER" };
        const wchar_t* types[] = { L"TYPE-A [BALANCE]", L"TYPE-B [POWER]", L"TYPE-C [SPEED]" };

        float centerX = scW / 2.0f;
        float nameWidth = wcslen(names[m_currentSelectIndex]) * 30.0f;

        uint32_t nameColor = 0xFFFFFFFF;
        if (m_currentSelectIndex == 0) nameColor = 0xFF00FFFF; // Cyan
        if (m_currentSelectIndex == 1) nameColor = 0xFFFFD700; // Gold
        if (m_currentSelectIndex == 2) nameColor = 0xFF00FF7F; // Green

        pGraphics->DrawString(names[m_currentSelectIndex], centerX - nameWidth / 2, panelY + 20, 48.0f, nameColor);

        float typeWidth = wcslen(types[m_currentSelectIndex]) * 15.0f;
        pGraphics->DrawString(types[m_currentSelectIndex], centerX - typeWidth / 2, panelY + 70, 24.0f, 0xFFCCCCCC);

        // --- ステータスバー ---
        int stats[3][3] = { {4, 3, 3}, {5, 5, 1}, {2, 4, 5} };

        float barStartX = 100.0f;
        float barStartY = panelY + 60.0f;
        float gapY = 40.0f;

        auto DrawStatBar = [&](const wchar_t* label, int val, int max, float y, uint32_t col) {
            pGraphics->DrawString(label, barStartX, y, 24.0f, 0xFFFFFFFF);
            float barsX = barStartX + 60.0f;
            for (int i = 0; i < max; ++i) {
                // 中身
                if (i < val) pGraphics->FillRect(barsX + i * 35.0f, y + 5, 30.0f, 15.0f, col);
                // 枠
                pGraphics->DrawRectOutline(barsX + i * 35.0f, y + 5, 30.0f, 15.0f, 1.0f, 0xFF444444);
            }
            };

        DrawStatBar(L"HP", stats[m_currentSelectIndex][0], 5, barStartY, 0xFF00FF00);
        DrawStatBar(L"ATK", stats[m_currentSelectIndex][1], 5, barStartY + gapY, 0xFFFF0000);
        DrawStatBar(L"SPD", stats[m_currentSelectIndex][2], 5, barStartY + gapY * 2, 0xFF00FFFF);

        // 説明文
        const wchar_t* descs[] = {
            L"Standard model equipped with a Beam Sword.\nEasy to handle for any pilot.",
            L"Heavy armor model with a Gravity Hammer.\nOverwhelming destructive power but slow.",
            L"High-mobility model with a Plasma Rifle.\nSnipes enemies from a distance."
        };
        pGraphics->DrawString(descs[m_currentSelectIndex], 650.0f, panelY + 60.0f, 24.0f, 0xFFEEEEEE);

        // 決定ボタン案内 (点滅)
        uint32_t pressColor = (static_cast<uint32_t>(alpha * 255.0f) << 24) | 0x00FFFFFF;
        pGraphics->DrawString(L"- PRESS ENTER to LAUNCH -", centerX - 180.0f, 680.0f, 32.0f, pressColor);

        pGraphics->EndDraw2D();
    }
}

void CharacterSelectScene::Shutdown() {
    pSkyBox.reset();
}

// プレビューモデルの生成
void CharacterSelectScene::UpdatePreviewModel() {
    // 1. 古いモデルとパーツを削除
    if (previewModelID != -1) {
        auto registry = pWorld->GetRegistry();
        std::vector<EntityID> partsToDelete;

        // パーツ検索
        for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
            if (registry->HasComponent<PlayerPartComponent>(id)) {
                if (registry->GetComponent<PlayerPartComponent>(id).parentID == previewModelID) {
                    partsToDelete.push_back(id);
                }
            }
        }
        // パーツ削除
        for (auto id : partsToDelete) pWorld->DestroyEntity(id);

        // 本体削除
        pWorld->DestroyEntity((EntityID)previewModelID);
        previewModelID = -1;
    }

    // 2. 新しいモデルを生成
    PlayerType type = (PlayerType)m_currentSelectIndex;

    // 見やすいサイズと位置に調整
    float scale = 0.5f;
    // Type Bは大きいので少し小さくする調整を入れるとバランスが良い
    if (type == PlayerType::BusterGuard) scale = 0.7f;

    if (pWorld) {
        EntitySpawnParams params;
        params.type = "Player";
        // 画面中央、少し下
        params.position = { 0.0f, -1.5f, 0.0f };
        params.scale = { scale, scale, scale };
        params.playerType = type;
        params.color = { 1, 1, 1, 1 };

        previewModelID = (int)EntityFactory::CreateEntity(pWorld.get(), params);

        // ---------------------------------------------------------
        // ★追加: アニメーションを「アイドル状態」にする設定
        // ---------------------------------------------------------
        // 生成直後は isGrounded が false (空中) になっている可能性があるため、
        // 強制的に true (接地) にして、待機モーションを再生させる。
        auto registry = pWorld->GetRegistry();
        if (registry->HasComponent<PlayerComponent>((EntityID)previewModelID)) {
            auto& pComp = registry->GetComponent<PlayerComponent>((EntityID)previewModelID);

            pComp.isGrounded = true;       // 接地扱いにする (これでIdleアニメになる)
            pComp.velocity = { 0, 0, 0 };  // 念のため速度もゼロに
            pComp.type = type;
        }
    }
}