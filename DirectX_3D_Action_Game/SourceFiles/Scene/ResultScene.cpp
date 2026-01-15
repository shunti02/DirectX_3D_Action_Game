#include "Scene/ResultScene.h"
#include "Scene/GameScene.h"
#include "Scene/TitleScene.h"
#include "Scene/CharacterSelectScene.h"
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

    // UI配置定義
    const float START_X = 400.0f;
    const float START_Y = 400.0f;
    const float ITEM_H = 60.0f;
    const float HIT_W = 400.0f;
    const float HIT_H = 40.0f;

    // ---------------------------------------------------------
    // 1. マウス操作 (動いた時のみ判定)
    // ---------------------------------------------------------
    // 左クリック決定は常に有効
    if (input->IsMouseKeyDown(0)) {
        // クリックされた位置が項目の上なら決定
        for (int i = 0; i < 3; ++i) {
            float itemY = START_Y + (i * ITEM_H);
            if (mx >= START_X && mx <= START_X + HIT_W &&
                my >= itemY && my <= itemY + HIT_H)
            {
                m_selectIndex = i; // 念のため選択も合わせる
                decided = true;
                break;
            }
        }
    }

    // カーソル移動 (マウスが動いた時だけ)
    if (isMouseMoved && !decided) {
        for (int i = 0; i < 3; ++i) {
            float itemY = START_Y + (i * ITEM_H);
            if (mx >= START_X && mx <= START_X + HIT_W &&
                my >= itemY && my <= itemY + HIT_H)
            {
                if (m_selectIndex != i) {
                    m_selectIndex = i;
                    // if (auto audio = ...) audio->Play("SE_SWITCH");
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
            if (m_selectIndex < 0) m_selectIndex = 2;
            // キー操作時はカーソル音を鳴らすなどの処理を入れると良い
        }
        if (input->IsKeyDown('S') || input->IsKeyDown(VK_DOWN)) {
            m_selectIndex++;
            if (m_selectIndex > 2) m_selectIndex = 0;
        }
        if (input->IsKeyDown(VK_RETURN) || input->IsKeyDown(VK_SPACE)) {
            decided = true;
        }
    }

    // ---------------------------------------------------------
    // 3. 決定時の遷移 (変更なし)
    // ---------------------------------------------------------
    if (decided) {
        if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_SWITCH");

        switch (m_selectIndex) {
        case 0: // RETRY
            // ★追加: リトライ時はステージの最初(フェーズ1)に戻し、HPも全快にする
            Game::GetInstance()->SetCurrentPhase(1);
            Game::GetInstance()->SetSavedHP(-1); // -1を指定すると全快スタートになる仕様

            Game::GetInstance()->GetSceneManager()->ChangeScene<GameScene>();
            break;

        case 1: // CUSTOMIZE
            // キャラ選択へ戻る (ここでも一応リセットしておくと安全です)
            Game::GetInstance()->SetCurrentPhase(1);
            Game::GetInstance()->SetSavedHP(-1);

            Game::GetInstance()->GetSceneManager()->ChangeScene<CharacterSelectScene>();
            break;

        case 2: // TITLE
            // タイトルへ (ここもリセット推奨)
            Game::GetInstance()->SetCurrentPhase(1);
            Game::GetInstance()->SetSavedHP(-1);

            Game::GetInstance()->GetSceneManager()->ChangeScene<TitleScene>();
            break;
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

    //メニュー選択肢
    const wchar_t* items[] = { L"RETRY MISSION", L"CUSTOMIZE SUIT", L"RETURN TO TITLE" };
    float startY = 400.0f;

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

    pGraphics->EndDraw2D();

    
}

void ResultScene::Shutdown() {
}