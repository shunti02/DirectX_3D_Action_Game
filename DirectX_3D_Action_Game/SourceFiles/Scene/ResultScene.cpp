#include "Scene/ResultScene.h"
#include "Scene/GameScene.h"
#include "Scene/TitleScene.h"
#include "Scene/CharacterSelectScene.h"
#include "Scene/StageSelectScene.h"
#include "App/Main.h"
#include "App/Game.h"
#include "Engine/Input.h"
bool ResultScene::isClear = false;

// コンストラクタ
ResultScene::ResultScene(SceneManager* manager)
    : BaseScene(manager)
{
}

void ResultScene::Initialize() {
    m_selectIndex = 0;

    if (isClear) {
        AppLog::AddLog("--- RESULT: GAME CLEAR ---");
        if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_JUMP");
    }
    else {
        AppLog::AddLog("--- RESULT: GAME OVER ---");
    }
}

void ResultScene::Update(float dt) {
    Input* input = Game::GetInstance()->GetInput();
    bool decided = false;

    // 現在のマウス座標
    DirectX::XMFLOAT2 mousePos = input->GetMousePosition();
    float mx = mousePos.x;
    float my = mousePos.y;

    // 前回のマウス座標を保存するための static 変数
    static float prevMx = -1.0f;
    static float prevMy = -1.0f;

    // マウスが動いたかどうか
    bool isMouseMoved = (abs(mx - prevMx) > 1.0f || abs(my - prevMy) > 1.0f);

    // 座標更新
    prevMx = mx;
    prevMy = my;

    // ★修正: メニュー項目数を状況によって変える
     // クリア時: [0:NEXT STAGE] [1:STAGE SELECT] [2:TITLE] (3つ)
     // 失敗時:   [0:RETRY] [1:TITLE] (2つ)
    int maxItems = isClear ? 3 : 2;

    // UI配置定義
    const float START_X = 400.0f;
    const float START_Y = 400.0f;
    const float ITEM_H = 60.0f;
    const float HIT_W = 400.0f;
    const float HIT_H = 40.0f;

    // ---------------------------------------------------------
    // 1. マウス操作 (動いた時のみ判定)
    // ---------------------------------------------------------
    if (input->IsMouseKeyDown(0)) {
        for (int i = 0; i < maxItems; ++i) { // 3ではなくmaxItemsを使用
            float itemY = START_Y + (i * ITEM_H);
            if (mx >= START_X && mx <= START_X + HIT_W &&
                my >= itemY && my <= itemY + HIT_H)
            {
                m_selectIndex = i;
                decided = true;
                break;
            }
        }
    }

    if (isMouseMoved && !decided) {
        for (int i = 0; i < maxItems; ++i) { // 3ではなくmaxItemsを使用
            float itemY = START_Y + (i * ITEM_H);
            if (mx >= START_X && mx <= START_X + HIT_W &&
                my >= itemY && my <= itemY + HIT_H)
            {
                if (m_selectIndex != i) {
                    m_selectIndex = i;
                }
            }
        }
    }

    // ---------------------------------------------------------
    // 2. キーボード操作
    // ---------------------------------------------------------
    if (!decided) {
        if (input->IsKeyDown('W') || input->IsKeyDown(VK_UP)) {
            m_selectIndex--;
            if (m_selectIndex < 0) m_selectIndex = maxItems - 1; // ループ
        }
        if (input->IsKeyDown('S') || input->IsKeyDown(VK_DOWN)) {
            m_selectIndex++;
            if (m_selectIndex >= maxItems) m_selectIndex = 0; // ループ
        }
        if (input->IsKeyDown(VK_RETURN) || input->IsKeyDown(VK_SPACE)) {
            decided = true;
        }
    }

    // ---------------------------------------------------------
    // 3. 決定時の遷移
    // ---------------------------------------------------------
    if (decided) {
        if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_SWITCH");

        if (isClear) {
            // ==========================================
            // ★GAME CLEAR時のメニュー処理
            // ==========================================
            switch (m_selectIndex) {
            case 0: // NEXT STAGE
            {
                int current = Game::GetInstance()->GetCurrentStage();
                // ステージ5未満なら次へ、5ならタイトルへ
                if (current < 5) {
                    Game::GetInstance()->SetCurrentStage(current + 1);
                    Game::GetInstance()->SetCurrentPhase(1);
                    Game::GetInstance()->SetSavedHP(-1); // 全快で開始
                    Game::GetInstance()->GetSceneManager()->ChangeScene<GameScene>();
                }
                else {
                    // 全クリ後の挙動 (エンディングがないのでタイトルへ)
                    Game::GetInstance()->GetSceneManager()->ChangeScene<TitleScene>();
                }
            }
            break;

            case 1: // STAGE SELECT (キャラ選択へ戻る)
                Game::GetInstance()->SetCurrentPhase(1);
                Game::GetInstance()->SetSavedHP(-1);
                Game::GetInstance()->GetSceneManager()->ChangeScene<StageSelectScene>();
                break;

            case 2: // TITLE
                Game::GetInstance()->GetSceneManager()->ChangeScene<TitleScene>();
                break;
            }
        }
        else {
            // ==========================================
            // ★GAME OVER時のメニュー処理
            // ==========================================
            switch (m_selectIndex) {
            case 0: // RETRY (同じステージの最初から)
                Game::GetInstance()->SetCurrentPhase(1);
                Game::GetInstance()->SetSavedHP(-1); // 全快でリトライ
                Game::GetInstance()->GetSceneManager()->ChangeScene<GameScene>();
                break;

            case 1: // TITLE
                Game::GetInstance()->GetSceneManager()->ChangeScene<TitleScene>();
                break;
            }
        }
    }
}

void ResultScene::Draw() {
    Graphics* pGraphics = Game::GetInstance()->GetGraphics();
    if (!pGraphics) return;

    pGraphics->BeginDraw2D();

    uint32_t bgColor = isClear ? 0xCC000033 : 0xCC330000;

    //結果表示
    if (isClear) {
        pGraphics->DrawString(L"MISSION ACCOMPLISHED", 150.0f, 150.0f, 64.0f, 0xFF00FFFF);
        pGraphics->DrawString(L"Excellent Work.", 450.0f, 250.0f, 32.0f, 0xFFFFFFFF);
    }
    else {
        pGraphics->DrawString(L"MISSION FAILED", 300.0f, 150.0f, 64.0f, 0xFFFF0000);
        pGraphics->DrawString(L"System Critical...", 450.0f, 250.0f, 32.0f, 0xFFAAAAAA);
    }

    // ★メニュー選択肢の描画 (分岐)
    float startY = 400.0f;

    if (isClear) {
        // --- クリア時 (3項目) ---
        const wchar_t* items[] = { L"NEXT STAGE", L"STAGE SELECT", L"RETURN TO TITLE" };
        for (int i = 0; i < 3; ++i) {
            bool isSelected = (m_selectIndex == i);
            uint32_t color = isSelected ? 0xFFFFFF00 : 0xFF808080;
            float size = isSelected ? 40.0f : 32.0f;
            float x = 400.0f + (isSelected ? 20.0f : 0.0f);

            pGraphics->DrawString(items[i], x, startY + (i * 60.0f), size, color);
            if (isSelected) {
                pGraphics->DrawString(L"→", x - 40.0f, startY + (i * 60.0f), size, 0xFFFF0000);
            }
        }
    }
    else {
        // --- 失敗時 (2項目) ---
        const wchar_t* items[] = { L"RETRY MISSION", L"RETURN TO TITLE" };
        for (int i = 0; i < 2; ++i) {
            bool isSelected = (m_selectIndex == i);
            uint32_t color = isSelected ? 0xFFFFFF00 : 0xFF808080;
            float size = isSelected ? 40.0f : 32.0f;
            float x = 400.0f + (isSelected ? 20.0f : 0.0f);

            pGraphics->DrawString(items[i], x, startY + (i * 60.0f), size, color);
            if (isSelected) {
                pGraphics->DrawString(L"→", x - 40.0f, startY + (i * 60.0f), size, 0xFFFF0000);
            }
        }
    }

    pGraphics->EndDraw2D();

    
}

void ResultScene::Shutdown() {
}