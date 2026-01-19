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
#include <cmath>
#include <vector>

// 選択状態を保持する変数 (cppファイルの先頭などで定義、またはクラスメンバ推奨)
static int titleMenuIndex = 0; // 0: NEW GAME, 1: CONTINUE
// パーティクル用構造体 (簡易)
struct Star {
    float x, y, speed, size;
};
static std::vector<Star> stars;

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

    // ★星の初期化 (100個)
    stars.clear();
    for (int i = 0; i < 100; ++i) {
        Star s;
        s.x = (float)(rand() % Config::SCREEN_WIDTH);
        s.y = (float)(rand() % Config::SCREEN_HEIGHT);
        s.speed = 2.0f + (rand() % 100) / 10.0f; // 2.0 ~ 12.0
        s.size = 1.0f + (rand() % 3);
        stars.push_back(s);
    }
}

void TitleScene::Update(float dt) {
    Input* input = Game::GetInstance()->GetInput();
    bool decided = false;
    // 背景をゆっくり回転させる
    cameraAngle += 0.2f * dt;

    // 文字点滅タイマー
    blinkTimer += dt;

    // ★星の移動更新 (右から左へ流れる)
    for (auto& s : stars) {
        s.x -= s.speed; // 左へ
        if (s.x < 0) {
            s.x = (float)Config::SCREEN_WIDTH;
            s.y = (float)(rand() % Config::SCREEN_HEIGHT);
        }
    }
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

    // メニュー配置 (左下に斜めに配置)
    const float MENU_START_X = 100.0f;
    const float MENU_START_Y = 400.0f;
    const float MENU_GAP_Y = 80.0f;
    const float MENU_OFFSET_X = 40.0f; // 斜めズレ

    // 項目は2つ (0: NEW GAME, 1: CONTINUE)
    for (int i = 0; i < 2; ++i) {
        float x = MENU_START_X + (i * MENU_OFFSET_X);
        float y = MENU_START_Y + (i * MENU_GAP_Y);

        // 当たり判定 (簡易矩形)
        if (mx >= x && mx <= x + 400.0f && my >= y && my <= y + 60.0f) {
            if (isMouseMoved && m_selectIndex != i) {
                m_selectIndex = i;
                if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_SWITCH");
            }
            if (input->IsMouseKeyDown(0)) decided = true;
        }
    }

    // ---------------------------------------------------------
    // 2. キーボード入力
    // ---------------------------------------------------------
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
        if (input->IsKeyDown(VK_RETURN) || input->IsKeyDown(VK_SPACE)) {
            decided = true;
        }
    }

    // ---------------------------------------------------------
    // 3. 遷移
    // ---------------------------------------------------------
    if (decided) {
        if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_JUMP");

        if (m_selectIndex == 0) { // NEW GAME
            Game::GetInstance()->ResetGame();
            Game::GetInstance()->GetSceneManager()->ChangeScene<CharacterSelectScene>();
        }
        else { // CONTINUE
            if (Game::GetInstance()->LoadGame()) {
                Game::GetInstance()->GetSceneManager()->ChangeScene<StageSelectScene>();
            }
            else {
                Game::GetInstance()->ResetGame();
                Game::GetInstance()->GetSceneManager()->ChangeScene<CharacterSelectScene>();
            }
        }
    }
}

void TitleScene::Draw() {
    Graphics* g = Game::GetInstance()->GetGraphics();

    // 1. SkyBox (背景: 宇宙)
    if (pSkyBox) {
        XMVECTOR eye = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
        XMVECTOR focus = XMVectorSet(sinf(cameraAngle), 0.0f, cosf(cameraAngle), 0.0f);
        XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        XMMATRIX view = XMMatrixLookAtLH(eye, focus, up);
        XMMATRIX proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), Config::SCREEN_WIDTH / (float)Config::SCREEN_HEIGHT, 0.1f, 1000.0f);
        pSkyBox->Draw(g, view, proj);
    }

    if (g) {
        g->BeginDraw2D();

        float scW = (float)Config::SCREEN_WIDTH;
        float scH = (float)Config::SCREEN_HEIGHT;

        // ★演出1: 流れる星 (Starfield)
        for (const auto& s : stars) {
            // 速度に応じて色を変える (速い＝青白く、遅い＝暗く)
            uint32_t col = (s.speed > 8.0f) ? 0xFFFFFFFF : 0xFF888888;
            g->FillRect(s.x, s.y, s.size * s.speed * 0.5f, s.size, col); // 横長に描画して軌跡っぽく
        }

        // ★演出2: グリッドライン (上下にサイバー空間の床/天井)
        float gridAlpha = (sinf(blinkTimer) + 1.0f) * 0.1f + 0.1f; // 0.1 ~ 0.3
        uint32_t gridCol = (static_cast<uint32_t>(gridAlpha * 255.0f) << 24) | 0x0000FFFF; // シアン

        // 遠近感を出すため、画面中央に向かって間隔を狭める
        for (int i = 0; i < 10; ++i) {
            float yTop = 200.0f * powf(0.8f, (float)i); // 上（天井）
            float yBot = scH - (200.0f * powf(0.8f, (float)i)); // 下（床）
            g->DrawRect(0, yTop, scW, 1.0f, gridCol);
            g->DrawRect(0, yBot, scW, 1.0f, gridCol);
        }

        // =========================================================
        // ロゴ描画 (グリッチエフェクト)
        // =========================================================
        float logoX = 600.0f; // 右側に配置
        float logoY = 200.0f;

        // グリッチ発生判定 (ランダム)
        bool isGlitch = (rand() % 100 < 5);
        float offsetX = isGlitch ? (rand() % 10 - 5.0f) : 0.0f;
        float offsetY = isGlitch ? (rand() % 10 - 5.0f) : 0.0f;
        uint32_t logoCol = isGlitch ? 0xFFFFFFFF : 0xFF00FFFF; // グリッチ時は白、通常はシアン

        // メインロゴ
        g->DrawString(L"SpaceButler", logoX + offsetX, logoY + offsetY, 80.0f, logoCol);
        // 残像 (RGBズレ表現)
        if (!isGlitch) {
            g->DrawString(L"SpaceButler", logoX + 4.0f, logoY, 80.0f, 0x88FF0000); // 赤ズレ
            g->DrawString(L"SpaceButler", logoX - 4.0f, logoY, 80.0f, 0x880000FF); // 青ズレ
        }

        g->DrawString(L"- CYBER FRONTIER -", logoX + 100.0f, logoY + 90.0f, 32.0f, 0xFFCCCCCC);


        // =========================================================
        // メニュー描画 (左下に斜め配置)
        // =========================================================
        const float MENU_START_X = 100.0f;
        const float MENU_START_Y = 400.0f;
        const float MENU_GAP_Y = 80.0f;
        const float MENU_OFFSET_X = 40.0f; // 下に行くほど右へ

        const wchar_t* items[] = { L"NEW GAME", L"CONTINUE" };

        for (int i = 0; i < 2; ++i) {
            bool isSelected = (m_selectIndex == i);
            float x = MENU_START_X + (i * MENU_OFFSET_X);
            float y = MENU_START_Y + (i * MENU_GAP_Y);

            // 選択されているとスライドするアニメーション
            if (isSelected) {
                x += 20.0f + sinf(blinkTimer * 5.0f) * 5.0f;
            }

            // 背景プレート (台形っぽく見せる装飾)
            float btnW = 400.0f;
            float btnH = 60.0f;
            uint32_t bgCol = isSelected ? 0xCC004488 : 0x88222222;
            uint32_t lineCol = isSelected ? 0xFF00FFFF : 0xFF444444;

            g->FillRect(x, y, btnW, btnH, bgCol);
            g->DrawRectOutline(x, y, btnW, btnH, 2.0f, lineCol);
            // 装飾ライン
            g->FillRect(x - 10.0f, y, 5.0f, btnH, lineCol);

            // 文字
            uint32_t textCol = isSelected ? 0xFFFFFFFF : 0xFFAAAAAA;
            g->DrawString(items[i], x + 30.0f, y + 10.0f, 40.0f, textCol);
        }

        // 下部メッセージ
        uint32_t pressColor = (static_cast<uint32_t>((sinf(blinkTimer * 4.0f) + 1.0f) * 0.5f * 255.0f) << 24) | 0x00FFFFFF;
        g->DrawString(L"SYSTEM READY...", 50.0f, scH - 50.0f, 24.0f, pressColor);

        g->EndDraw2D();
    }
}