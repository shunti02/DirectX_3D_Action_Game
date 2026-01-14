/*===================================================================
// ファイル: TitleScene.cpp
// 概要: 豪華になったタイトル画面
=====================================================================*/
#include "Scene/TitleScene.h"
#include "App/Game.h"
#include "Scene/GameScene.h"
#include "App/Main.h"
#include "../ImGui/imgui.h"

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
}

void TitleScene::Update(float dt) {
    //if (Game::GetInstance()->GetInput()->IsKeyDown(VK_RETURN)) {
    //    Game::GetInstance()->GetSceneManager()->ChangeScene<GameScene>();
    //}
    Input* input = Game::GetInstance()->GetInput();

    // 背景をゆっくり回転させる
    cameraAngle += 0.1f * dt;

    // 文字点滅タイマー
    blinkTimer += dt;

    // スペースキーでゲーム開始
    if (input->IsKeyDown(VK_SPACE)) {
        // SE再生
        if (auto audio = Game::GetInstance()->GetAudio()) {
            audio->Play("SE_JUMP"); // 決定音の代わりにJUMP音を鳴らす（仮）
            // BGMを止めるならここで audio->StopAll();
        }

        // シーン遷移
        Game::GetInstance()->GetSceneManager()->ChangeScene<GameScene>();
    }
}

void TitleScene::Draw() {
    /*ImGui::SetNextWindowPos(ImVec2(Config::SCREEN_WIDTH / 2 - 100, Config::SCREEN_HEIGHT / 2 - 50));
    ImGui::SetNextWindowSize(ImVec2(200, 100));
    ImGui::Begin("Title", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground);

    ImGui::SetWindowFontScale(2.0f);
    ImGui::Text("ACTION GAME");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Text("Press ENTER to Start");

    ImGui::End();*/
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

        // 点滅する "PRESS SPACE"
        // sin波を使って透明度を変化させる (0.0 ~ 1.0)
        float alpha = (sinf(blinkTimer * 5.0f) + 1.0f) * 0.5f;
        uint32_t alphaInt = static_cast<uint32_t>(alpha * 255.0f);
        uint32_t color = (alphaInt << 24) | 0x00FFFF00; // 黄色 + アルファ

        pGraphics->DrawString(L"PRESS SPACE TO START", 430.0f, 500.0f, 32.0f, color);

        pGraphics->EndDraw2D();
    }
}