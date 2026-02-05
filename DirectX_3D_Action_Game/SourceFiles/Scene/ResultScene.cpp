#define NOMINMAX
#include "Scene/ResultScene.h"
#include "Scene/GameScene.h"
#include "Scene/TitleScene.h"
#include "Scene/CharacterSelectScene.h"
#include "Scene/StageSelectScene.h"
#include "App/Main.h"
#include "App/Game.h"
#include "Engine/Input.h"
#include <algorithm>
#include <cmath>

bool ResultScene::isClear = false;
// ★演出用変数
static float rs_blinkTimer = 0.0f;
// コンストラクタ
ResultScene::ResultScene(SceneManager* manager)
    : BaseScene(manager)
{
}

void ResultScene::Initialize() {
    m_selectIndex = 0;
    rs_blinkTimer = 0.0f;
    if (isClear) {
        AppLog::AddLog("--- RESULT: GAME CLEAR ---");
        if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_JUMP");
    }
    else {
        AppLog::AddLog("--- RESULT: GAME OVER ---");
    }
}

void ResultScene::Update(float dt) {
    rs_blinkTimer += dt;
    Input* input = Game::GetInstance()->GetInput();
    bool decided = false;

    // 現在のマウス座標
    DirectX::XMFLOAT2 mousePos = input->GetMousePosition();
    float mx = mousePos.x;
    float my = mousePos.y;

    // 前回のマウス座標を保存
    static float prevMx = -1.0f;
    static float prevMy = -1.0f;
    bool isMouseMoved = (abs(mx - prevMx) > 1.0f || abs(my - prevMy) > 1.0f);
    prevMx = mx;
    prevMy = my;

    // ★修正: メニュー項目数を状況によって変える
     // クリア時: [0:NEXT STAGE] [1:STAGE SELECT] [2:TITLE] (3つ)
     // 失敗時:   [0:RETRY] [1:TITLE] (2つ)
    int maxItems = isClear ? 3 : 2;

    // ★修正: 斜め配置に合わせたマウス判定
    // Draw関数と同じパラメータ定義
    const float MENU_START_X = 100.0f;
    const float MENU_START_Y = 400.0f; // 画面下部寄り
    const float MENU_GAP_Y = 80.0f;
    const float MENU_OFFSET_X = 40.0f; // 斜めズレ
    const float BTN_W = 400.0f;
    const float BTN_H = 60.0f;

    int hoverIndex = -1;
    bool isHover = false;

    for (int i = 0; i < maxItems; ++i) {
        float x = MENU_START_X + (i * MENU_OFFSET_X);
        float y = MENU_START_Y + (i * MENU_GAP_Y);

        // 簡易矩形判定 (スライド分は考慮せず)
        if (mx >= x && mx <= x + BTN_W && my >= y && my <= y + BTN_H) {
            hoverIndex = i;
            isHover = true;
            break;
        }
    }

    // マウス操作
    if (isHover) {
        if (isMouseMoved && m_selectIndex != hoverIndex) {
            m_selectIndex = hoverIndex;
            if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_SWITCH");
        }
        if (input->IsMouseKeyDown(0)) {
            m_selectIndex = hoverIndex;
            decided = true;
        }
    }

    // キーボード操作
    if (!decided) {
        if (input->IsKeyDown('W') || input->IsKeyDown(VK_UP)) {
            m_selectIndex--;
            if (m_selectIndex < 0) m_selectIndex = maxItems - 1;
            if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_SWITCH");
        }
        if (input->IsKeyDown('S') || input->IsKeyDown(VK_DOWN)) {
            m_selectIndex++;
            if (m_selectIndex >= maxItems) m_selectIndex = 0;
            if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_SWITCH");
        }
        if (input->IsKeyDown(VK_RETURN) || input->IsKeyDown(VK_SPACE)) {
            decided = true;
        }
    }

    // 決定時の遷移
    if (decided) {
        if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_JUMP");

        if (isClear) {
            switch (m_selectIndex) {
            case 0: // NEXT
                if (Game::GetInstance()->GetCurrentStage() < 5) {
                    Game::GetInstance()->SetCurrentStage(Game::GetInstance()->GetCurrentStage() + 1);
                    Game::GetInstance()->SetCurrentPhase(1);
                    Game::GetInstance()->SetSavedHP(-1);
                    Game::GetInstance()->GetSceneManager()->ChangeScene<GameScene>();
                }
                else {
                    Game::GetInstance()->GetSceneManager()->ChangeScene<TitleScene>();
                }
                break;
            case 1: // SELECT
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
            switch (m_selectIndex) {
            case 0: // RETRY
                Game::GetInstance()->SetCurrentPhase(1);
                Game::GetInstance()->SetSavedHP(-1);
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

    // --- メインメッセージ ---
    float scW = (float)Config::SCREEN_WIDTH;
    float scH = (float)Config::SCREEN_HEIGHT;
    float cx = scW / 2.0f;
    float cy = scH / 2.0f;
    // --- 背景色 (オーバーレイ) ---
    uint32_t overlayColor = isClear ? 0xAA001133 : 0xAA330000;
    pGraphics->FillRect(0, 0, scW, scH, overlayColor);

    // ★スキャンライン (タイトル共通)
    float gridAlpha = (sinf(rs_blinkTimer) + 1.0f) * 0.1f + 0.1f;
    uint32_t gridCol = (static_cast<uint32_t>(gridAlpha * 255.0f) << 24) | (isClear ? 0x0000FFFF : 0x00FF0000); // 青or赤

    for (float y = 0; y < scH; y += 4.0f) {
        pGraphics->DrawRect(0, y, scW, 1.0f, 0x22000000);
    }
    // 上下のグリッドライン演出
    for (int i = 0; i < 5; ++i) {
        float yTop = 150.0f * powf(0.8f, (float)i);
        float yBot = scH - (150.0f * powf(0.8f, (float)i));
        pGraphics->DrawRect(0, yTop, scW, 1.0f, gridCol);
        pGraphics->DrawRect(0, yBot, scW, 1.0f, gridCol);
    }
    // --- 結果ロゴ (グリッチ演出) ---
    float scale = 1.0f + sinf(rs_blinkTimer * 3.0f) * 0.05f; // 鼓動
    bool isGlitch = (rand() % 100 < 5);
    float offX = isGlitch ? (rand() % 10 - 5.0f) : 0.0f;
    float offY = isGlitch ? (rand() % 10 - 5.0f) : 0.0f;

    if (isClear) {
        // [MISSION ACCOMPLISHED]
        uint32_t col = isGlitch ? 0xFFFFFFFF : 0xFF00FFFF;
        float logoY = 150.0f;
        pGraphics->DrawString(L"MISSION ACCOMPLISHED", cx - 350.0f + offX, logoY + offY, 64.0f * scale, col);
        if (!isGlitch) {
            pGraphics->DrawString(L"MISSION ACCOMPLISHED", cx - 346.0f, logoY, 64.0f * scale, 0x880000FF);
        }

        pGraphics->DrawString(L"- TARGET DESTROYED -", cx - 180.0f, logoY + 80.0f, 32.0f, 0xFFFFFFFF);
        pGraphics->DrawRectOutline(0, logoY - 20.0f, scW, 140.0f, 2.0f, 0xFF00FFFF);
    }
    else {
        // [MISSION FAILED]
        uint32_t col = isGlitch ? 0xFFFFFFFF : 0xFFFF0000;
        float logoY = 150.0f;
        pGraphics->DrawString(L"MISSION FAILED", cx - 250.0f + offX, logoY + offY, 64.0f * scale, col);
        if (!isGlitch) {
            pGraphics->DrawString(L"MISSION FAILED", cx - 246.0f, logoY, 64.0f * scale, 0x88FF0000);
        }

        pGraphics->DrawString(L"- SIGNAL LOST -", cx - 120.0f, logoY + 80.0f, 32.0f, 0xFFAAAAAA);
        pGraphics->DrawRectOutline(0, logoY - 20.0f, scW, 140.0f, 2.0f, 0xFFFF0000);
    }

    // --- メニューボタン (タイトル画面と同じ斜めスライド式) ---
    const float MENU_START_X = 100.0f;
    const float MENU_START_Y = 400.0f;
    const float MENU_GAP_Y = 80.0f;
    const float MENU_OFFSET_X = 40.0f;
    const float BTN_W = 400.0f;
    const float BTN_H = 60.0f;

    std::vector<const wchar_t*> items;
    if (isClear) items = { L"NEXT STAGE", L"STAGE SELECT", L"TITLE" };
    else items = { L"RETRY", L"TITLE" };

    for (size_t i = 0; i < items.size(); ++i) {
        bool isSelected = (m_selectIndex == (int)i);
        float x = MENU_START_X + (i * MENU_OFFSET_X);
        float y = MENU_START_Y + (i * MENU_GAP_Y);

        if (isSelected) {
            x += 20.0f + sinf(rs_blinkTimer * 5.0f) * 5.0f; // スライド

            uint32_t bgCol = isClear ? 0xCC004488 : 0xCC440000;
            uint32_t lineCol = isClear ? 0xFF00FFFF : 0xFFFF0000;

            pGraphics->FillRect(x, y, BTN_W, BTN_H, bgCol);
            pGraphics->DrawRectOutline(x, y, BTN_W, BTN_H, 2.0f, lineCol);
            // 装飾
            pGraphics->FillRect(x - 10.0f, y, 5.0f, BTN_H, lineCol);

            pGraphics->DrawString(items[i], x + 30.0f, y + 10.0f, 32.0f, 0xFFFFFFFF);
        }
        else {
            pGraphics->FillRect(x, y, BTN_W, BTN_H, 0x88222222);
            pGraphics->DrawRectOutline(x, y, BTN_W, BTN_H, 1.0f, 0xFF666666);
            pGraphics->DrawString(items[i], x + 30.0f, y + 10.0f, 32.0f, 0xFFAAAAAA);
        }
    }

    pGraphics->EndDraw2D();
}

void ResultScene::Shutdown() {
}