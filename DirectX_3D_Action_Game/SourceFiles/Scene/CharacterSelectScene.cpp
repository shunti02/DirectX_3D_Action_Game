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

    // プレビューモデル生成
    UpdatePreviewModel();

    AppLog::AddLog("--- Character Select Scene (Hybrid Input) ---");
}

void CharacterSelectScene::Update(float dt) {
    Input* input = Game::GetInstance()->GetInput();
    bool changed = false;
    bool decided = false;

    // ---------------------------------------------------------
    // 1. マウス操作の判定
    // ---------------------------------------------------------
    DirectX::XMFLOAT2 mousePos = input->GetMousePosition();
    float mx = mousePos.x;
    float my = mousePos.y;

    // UI配置 (Drawと合わせる)
    const float START_X = 100.0f;
    const float START_Y = 200.0f;
    const float ITEM_H = 60.0f;
    const float HIT_W = 400.0f;
    const float HIT_H = 40.0f;

    for (int i = 0; i < 3; ++i) {
        float itemY = START_Y + (i * ITEM_H);

        // 当たり判定
        if (mx >= START_X && mx <= START_X + HIT_W &&
            my >= itemY && my <= itemY + HIT_H)
        {
            if (m_currentSelectIndex != i) {
                m_currentSelectIndex = i;
                changed = true;
            }
            if (input->IsMouseKeyDown(0)) { // 左クリック
                decided = true;
            }
        }
    }

    // ---------------------------------------------------------
    // 2. キーボード操作の判定 (マウス操作がない場合)
    // ---------------------------------------------------------
    if (!changed && !decided) { // マウスで変化がなければキー入力を受付
        if (input->IsKeyDown('A') || input->IsKeyDown(VK_LEFT) || input->IsKeyDown('W') || input->IsKeyDown(VK_UP)) {
            m_currentSelectIndex--;
            if (m_currentSelectIndex < 0) m_currentSelectIndex = 2;
            changed = true;
        }
        if (input->IsKeyDown('D') || input->IsKeyDown(VK_RIGHT) || input->IsKeyDown('S') || input->IsKeyDown(VK_DOWN)) {
            m_currentSelectIndex++;
            if (m_currentSelectIndex > 2) m_currentSelectIndex = 0;
            changed = true;
        }
        // キーボードでの決定
        if (input->IsKeyDown(VK_RETURN) || input->IsKeyDown(VK_SPACE)) {
            decided = true;
        }
    }

    // ---------------------------------------------------------
    // 3. 状態更新と遷移
    // ---------------------------------------------------------
    // 選択が変わった場合
    if (changed) {
        UpdatePreviewModel();
        // カーソル音 (あれば)
        // if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_CURSOR");
    }

    // プレビューモデルを回す
    if (previewModelID != -1) {
        auto& trans = pWorld->GetComponent<TransformComponent>((EntityID)previewModelID);
        trans.rotation.y += 1.0f * dt;
    }

    // 決定時の処理
    if (decided) {
        Game::GetInstance()->SetPlayerType((PlayerType)m_currentSelectIndex);
        Game::GetInstance()->SetCurrentStage(1);
        Game::GetInstance()->GetSceneManager()->ChangeScene<StageSelectScene>();

        if (auto audio = Game::GetInstance()->GetAudio()) {
            audio->Play("SE_JUMP"); // 決定音
        }
    }

    // ECS更新 (モデル描画のため)
    if (pWorld) pWorld->Update(dt);
}

void CharacterSelectScene::Draw() {
    Graphics* pGraphics = Game::GetInstance()->GetGraphics();

    // 背景描画
    if (pSkyBox) {
        // カメラ設定 (固定)
        XMVECTOR eye = DirectX::XMVectorSet(0.0f, 1.0f, -5.0f, 0.0f);
        XMVECTOR focus = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        XMMATRIX view = DirectX::XMMatrixLookAtLH(eye, focus, up);
        XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(45.0f),
            Config::SCREEN_WIDTH / (float)Config::SCREEN_HEIGHT, 0.1f, 1000.0f);
        pSkyBox->Draw(pGraphics, view, proj);
    }

    // モデル描画
    BaseScene::Draw();

    // UI描画 (Direct2D)
    if (pGraphics) {
        pGraphics->BeginDraw2D();

        pGraphics->DrawString(L"SELECT YOUR EXO-SUIT", 100.0f, 50.0f, 48.0f, 0xFFFFFFFF);
        pGraphics->DrawString(L"Mouse Click or Press [Enter]", 100.0f, 110.0f, 24.0f, 0xFFCCCCCC);

        const wchar_t* names[] = { L"TYPE A: Assault Striker", L"TYPE B: Buster Guard", L"TYPE C: Plasma Sniper" };
        const wchar_t* descs[] = {
            L"Weapon: Beam Sword\n[Balance Type]",
            L"Weapon: Gravity Hammer\n[Power Type]",
            L"Weapon: Plasma Rifle\n[Range Type]"
        };

        float startY = 200.0f;
        for (int i = 0; i < 3; i++) {
            bool isSelected = (m_currentSelectIndex == i);
            uint32_t color = isSelected ? 0xFF00FFFF : 0xFF808080;
            float size = isSelected ? 40.0f : 32.0f;
            float x = 100.0f + (isSelected ? 20.0f : 0.0f);

            pGraphics->DrawString(names[i], x, startY + (i * 60.0f), size, color);

            if (isSelected) {
                pGraphics->DrawString(L"→", x - 40.0f, startY + (i * 60.0f), size, 0xFFFF0000);
            }
        }

        pGraphics->DrawString(descs[m_currentSelectIndex], 600.0f, 250.0f, 28.0f, 0xFFFFFFFF);
        pGraphics->DrawString(L"LAUNCH MISSION", 450.0f, 600.0f, 48.0f, 0xFFFFFF00);

        pGraphics->EndDraw2D();
    }
}

void CharacterSelectScene::Shutdown() {
    pSkyBox.reset();
}

// プレビューモデルの生成
void CharacterSelectScene::UpdatePreviewModel() {
    if (previewModelID != -1) {
        pWorld->DestroyEntity((EntityID)previewModelID);
    }

    PlayerType type = (PlayerType)m_currentSelectIndex;
    DirectX::XMFLOAT4 color;
    float scale = 1.0f;

    switch (type) {
    case PlayerType::AssaultStriker:
        color = { 0.0f, 0.8f, 1.0f, 1.0f };
        break;
    case PlayerType::BusterGuard:
        color = { 1.0f, 0.8f, 0.0f, 1.0f };
        scale = 1.4f;
        break;
    case PlayerType::PlasmaSniper:
        color = { 0.2f, 1.0f, 0.4f, 1.0f };
        break;
    }

    if (pWorld) {
        EntitySpawnParams params;
        params.type = "Player";
        params.position = { 0.0f, -1.0f, 0.0f };
        params.scale = { scale, scale, scale };
        params.color = color;
        previewModelID = (int)EntityFactory::CreateEntity(pWorld.get(), params);
    }
}