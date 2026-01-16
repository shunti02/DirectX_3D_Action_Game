/*===================================================================
// ファイル: TitleScene.cpp
// 概要: 豪華になったタイトル画面
=====================================================================*/
#include "Scene/TitleScene.h"
#include "App/Game.h"
#include "Scene/GameScene.h"
#include "Scene/CharacterSelectScene.h"
#include "Scene/StageSelectScene.h"
#include "App/Main.h"
#include "../ImGui/imgui.h"
#include "Engine/Input.h"

// 選択状態を保持する変数 (cppファイルの先頭などで定義、またはクラスメンバ推奨)
static int titleMenuIndex = 0; // 0: NEW GAME, 1: CONTINUE

void TitleScene::Initialize() {
    // 1. SkyBox初期化
    pSkyBox = std::make_unique<SkyBox>();
    if (!pSkyBox->Initialize(Game::GetInstance()->GetGraphics())) {
        pSkyBox.reset();
    }

    // 2. 文字描画用システム (Initializeは不要な簡易利用)
    pUISystem = std::make_unique<UISystem>();

    // 3. BGM再生
    // ※Game::Initializeでロード済みであることが前提
    if (auto audio = Game::GetInstance()->GetAudio()) {
        // 音量0.5でループ再生
        audio->Play("BGM_TITLE", true, 0.5f);
    }
    m_selectIndex = 0; // 初期選択は NEW GAME
}

void TitleScene::Update(float dt) {
    Input* input = Game::GetInstance()->GetInput();
    bool decided = false;
    // 背景をゆっくり回転させる
    cameraAngle += 0.1f * dt;

    // 文字点滅タイマー
    blinkTimer += dt;

    // ---------------------------------------------------------
    // 1. マウス入力処理
    // ---------------------------------------------------------
    DirectX::XMFLOAT2 mousePos = input->GetMousePosition();
    float mx = mousePos.x;
    float my = mousePos.y;

    // マウスが動いたか判定するためのstatic変数
    static float prevMx = -1.0f;
    static float prevMy = -1.0f;
    bool isMouseMoved = (abs(mx - prevMx) > 1.0f || abs(my - prevMy) > 1.0f);
    prevMx = mx;
    prevMy = my;

    // UI配置定義 (Draw関数の座標と合わせる)
    const float MENU_START_X = 480.0f; // 文字の左端
    const float MENU_START_Y = 450.0f; // 1つ目の項目のY
    const float MENU_ITEM_H = 60.0f;   // 行間
    const float HIT_W = 300.0f;        // 当たり判定の幅
    const float HIT_H = 40.0f;         // 当たり判定の高さ

    // 項目は2つ (0: NEW GAME, 1: CONTINUE)
    for (int i = 0; i < 2; ++i) {
        float itemY = MENU_START_Y + (i * MENU_ITEM_H);

        // マウスカーソルが文字の上にあるか？
        if (mx >= MENU_START_X && mx <= MENU_START_X + HIT_W &&
            my >= itemY && my <= itemY + HIT_H)
        {
            // マウスが動いていれば選択状態を更新
            if (isMouseMoved && m_selectIndex != i) {
                m_selectIndex = i;
                if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_SWITCH");
            }
            // クリックで決定
            if (input->IsMouseKeyDown(0)) { // 左クリック
                decided = true;
            }
        }
    }

    // ---------------------------------------------------------
    // 2. キーボード入力処理
    // ---------------------------------------------------------
    // まだ決定していなければキー操作を受け付ける
    if (!decided) {
        if (input->IsKeyDown(VK_UP) || input->IsKeyDown('W')) {
            m_selectIndex--;
            if (m_selectIndex < 0) m_selectIndex = 1;
            if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_SWITCH");
        }
        if (input->IsKeyDown(VK_DOWN) || input->IsKeyDown('S')) {
            m_selectIndex++;
            if (m_selectIndex > 1) m_selectIndex = 0;
            if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_SWITCH");
        }
        // エンターまたはスペースで決定
        if (input->IsKeyDown(VK_RETURN) || input->IsKeyDown(VK_SPACE)) {
            decided = true;
        }
    }

    // ---------------------------------------------------------
    // 3. 決定時の遷移
    // ---------------------------------------------------------
    if (decided) {
        if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_JUMP");

        if (m_selectIndex == 0) {
            // [NEW GAME]
            Game::GetInstance()->ResetGame();
            // キャラ選択へ
            Game::GetInstance()->GetSceneManager()->ChangeScene<CharacterSelectScene>();
        }
        else {
            // [CONTINUE]
            if (Game::GetInstance()->LoadGame()) {
                // セーブデータがあればステージ選択へ
                Game::GetInstance()->GetSceneManager()->ChangeScene<StageSelectScene>();
            }
            else {
                // データがなければ NEW GAME 扱い (またはブザー音を鳴らして拒否も可)
                Game::GetInstance()->ResetGame();
                Game::GetInstance()->GetSceneManager()->ChangeScene<CharacterSelectScene>();
            }
        }
    }
}

void TitleScene::Draw() {
    Graphics* pGraphics = Game::GetInstance()->GetGraphics();

    // 1. SkyBox描画 (カメラを回す)
    if (pSkyBox) {
        // カメラの位置は原点、見る方向を回転させる
        XMVECTOR eye = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f); // 中心
        XMVECTOR focus = XMVectorSet(sinf(cameraAngle), 0.0f, cosf(cameraAngle), 0.0f); // 回転
        XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

        XMMATRIX view = XMMatrixLookAtLH(eye, focus, up);
        XMMATRIX proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), Config::SCREEN_WIDTH / (float)Config::SCREEN_HEIGHT, 0.1f, 1000.0f);

        pSkyBox->Draw(pGraphics, view, proj);
    }

    // 2. 文字描画 (Direct2D)
    if (pGraphics) {
        pGraphics->BeginDraw2D();

        // タイトルロゴ
        pGraphics->DrawString(L"SpaceButler", 450.0f, 200.0f, 64.0f, 0xFF00FFFF); // シアン色
        //pGraphics->DrawString(L"- 宇宙の戦士 -", 520.0f, 280.0f, 32.0f, 0xFFFFFFFF);

       // --- メニュー項目の描画 ---
        // Updateの当たり判定と同じ座標定数を使用
        const float MENU_X = 500.0f; // 表示位置 (当たり判定より少し右)
        const float MENU_START_Y = 450.0f;
        const float MENU_ITEM_H = 60.0f;

        const wchar_t* items[] = { L"NEW GAME", L"CONTINUE" };

        for (int i = 0; i < 2; ++i) {
            bool isSelected = (m_selectIndex == i);

            // 選択されていたら黄色、それ以外はグレー
            uint32_t color = isSelected ? 0xFFFFFF00 : 0xFF888888;
            float size = isSelected ? 42.0f : 40.0f; // 選択中は少し大きく

            float y = MENU_START_Y + (i * MENU_ITEM_H);

            // 文字描画
            pGraphics->DrawString(items[i], MENU_X, y, size, color);

            // 矢印描画
            if (isSelected) {
                pGraphics->DrawString(L"→", MENU_X - 40.0f, y, size, 0xFFFF0000);
            }
        }

        // 下部の案内メッセージ (点滅)
        float alpha = (sinf(blinkTimer * 5.0f) + 1.0f) * 0.5f;
        uint32_t alphaInt = static_cast<uint32_t>(alpha * 255.0f);
        uint32_t pressColor = (alphaInt << 24) | 0x00FFFFFF;
        pGraphics->DrawString(L"- PRESS ENTER or CLICK -", 480.0f, 650.0f, 24.0f, pressColor);

        pGraphics->EndDraw2D();
    }
}