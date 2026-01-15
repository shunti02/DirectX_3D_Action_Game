#include "Scene/StageSelectScene.h"
#include "Scene/GameScene.h"
#include "App/Main.h"
#include "Engine/Input.h"

StageSelectScene::StageSelectScene(SceneManager* manager) : BaseScene(manager) {}

void StageSelectScene::Initialize() {
    pSkyBox = std::make_unique<SkyBox>();
    pSkyBox->Initialize(Game::GetInstance()->GetGraphics());

    // カーソル初期位置は最新のステージに
    m_selectIndex = Game::GetInstance()->GetMaxUnlockedStage() - 1;
    AppLog::AddLog("--- Stage Select ---");
}

void StageSelectScene::Update(float dt) {
    Input* input = Game::GetInstance()->GetInput();
    int maxUnlock = Game::GetInstance()->GetMaxUnlockedStage();
    bool decided = false;

    // --- マウス操作 ---
    DirectX::XMFLOAT2 mousePos = input->GetMousePosition();
    float mx = mousePos.x;
    float my = mousePos.y;

    // ボタン配置定義
    float startX = 100.0f;
    float startY = 200.0f;
    float gapY = 80.0f;

    for (int i = 0; i < 5; ++i) {
        // ロックされているステージは判定しない
        if (i >= maxUnlock) continue;

        if (mx >= startX && mx <= startX + 400.0f &&
            my >= startY + (i * gapY) && my <= startY + (i * gapY) + 50.0f)
        {
            if (m_selectIndex != i) m_selectIndex = i;
            if (input->IsMouseKeyDown(0)) decided = true;
        }
    }

    // --- キーボード操作 ---
    if (!decided) {
        if (input->IsKeyDown('W') || input->IsKeyDown(VK_UP)) {
            m_selectIndex--;
            if (m_selectIndex < 0) m_selectIndex = maxUnlock - 1;
        }
        if (input->IsKeyDown('S') || input->IsKeyDown(VK_DOWN)) {
            m_selectIndex++;
            if (m_selectIndex >= maxUnlock) m_selectIndex = 0;
        }
        if (input->IsKeyDown(VK_RETURN) || input->IsKeyDown(VK_SPACE)) {
            decided = true;
        }
    }

    // --- 決定 ---
    if (decided) {
        if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_JUMP");

        // ステージ設定 & フェーズ1から開始 & HP全快(-1)
        Game::GetInstance()->SetCurrentStage(m_selectIndex + 1);
        Game::GetInstance()->SetCurrentPhase(1);
        Game::GetInstance()->SetSavedHP(-1);

        Game::GetInstance()->GetSceneManager()->ChangeScene<GameScene>();
    }
}

void StageSelectScene::Draw() {
    Graphics* g = Game::GetInstance()->GetGraphics();

    // 背景
    XMMATRIX view = XMMatrixLookAtLH(XMVectorSet(0, 0, -5, 0), XMVectorSet(0, 0, 0, 0), XMVectorSet(0, 1, 0, 0));
    XMMATRIX proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(45), 1280.0f / 720.0f, 0.1f, 1000.0f);
    if (pSkyBox) pSkyBox->Draw(g, view, proj);

    g->BeginDraw2D();
    g->DrawString(L"SELECT MISSION", 100, 50, 48, 0xFFFFFFFF);

    int maxUnlock = Game::GetInstance()->GetMaxUnlockedStage();
    float startX = 100.0f;
    float startY = 200.0f;

    for (int i = 0; i < 5; ++i) {
        uint32_t color = 0xFF808080; // ロック中（グレー）
        std::wstring text = L"STAGE " + std::to_wstring(i + 1) + L": LOCKED";

        if (i < maxUnlock) {
            // 解放済み
            text = L"STAGE " + std::to_wstring(i + 1);
            if (i == 4) text += L" [FINAL OPERATION]";
            else text += L" [NORMAL MISSION]";

            if (i == m_selectIndex) {
                color = 0xFF00FFFF; // 選択中（シアン）
                g->DrawString(L"→", startX - 40, startY + (i * 80), 32, 0xFFFF0000);
            }
            else {
                color = 0xFFFFFFFF; // 白
            }
        }

        g->DrawString(text.c_str(), startX, startY + (i * 80), 32, color);
    }

    g->EndDraw2D();
}

void StageSelectScene::Shutdown() {
    pSkyBox.reset();
}