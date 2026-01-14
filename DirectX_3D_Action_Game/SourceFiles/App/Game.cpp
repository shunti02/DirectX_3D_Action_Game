/*===================================================================
//ファイル:Game.cpp
//概要:Gameクラスの実装
=====================================================================*/
#include "App/Game.h"
#include "App/Main.h"
#include "Scene/GameScene.h"
#include "Scene/TitleScene.h"
#include "../ImGui/imgui.h"

Game* Game::instance = nullptr;

Game::Game() {
    instance = this;
    pSceneManager = std::make_unique<SceneManager>();
    pInput = std::make_unique<Input>();
}

Game::~Game() {
    Shutdown();
    instance = nullptr;
}

bool Game::Initialize(HWND hWnd) {
    pInput->Initialize();
    // Graphics初期化
    pGraphics = std::make_unique<Graphics>();
    if (!pGraphics->Initialize(hWnd, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT)) {
        return false;
    }

	// ImGui初期化
	pGraphics->InitUI(hWnd);

    //Audio初期化
    pAudio = std::make_unique<Audio>();
    if (!pAudio->Initialize()) {
        AppLog::AddLog("[Warning] Audio Initialize Failed.");
    }
    else {
        AppLog::AddLog("[Audio] Audio Initialized.");

        pAudio->LoadSound("SE_JUMP", L"Assets/Audio/jump.wav");
        pAudio->LoadSound("SE_SWITCH", L"Assets/Audio/switch.wav");
        pAudio->LoadSound("BGM_TITLE", L"Assets/Audio/title_bgm.wav");
    }

    // 最初のシーンへ遷移 (GameScene)
    pSceneManager->ChangeScene<TitleScene>();

    return true;
}

void Game::Update(float dt) {
    pInput->Update();

    if (pAudio) {
        pAudio->Update();
    }
    // ImGui描画開始
    pGraphics->BeginUI();

    if (pSceneManager) {
        pSceneManager->Update(dt);
    }
}

void Game::Draw() {
    if (!pGraphics) return;

    // 画面クリア（背景色：ダークグレー）
    pGraphics->BeginFrame(0.1f, 0.1f, 0.1f, 1.0f);

	

    // シーン描画
    if (pSceneManager) {
        pSceneManager->Draw();
    }

    //デバッグUIのテスト表示
	ImGui::Begin("Debug Menu");//ウィンドウ表示
	
    ImGui::Text("Hello, ImGui!");//テキスト表示
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate); // FPS表示
    // 背景色を変えるテスト
    static float color[4] = { 0.1f,0.1f,0.1f,1.0f };
	ImGui::ColorEdit3("BG Color", color);//カラーパレット表示
    //ウィンドウ終了
    ImGui::End();

    //UI描画終了
    pGraphics->EndUI();

    // 画面フリップ
    pGraphics->EndFrame();
}

void Game::Shutdown() {
    pSceneManager.reset();
    pAudio.reset();
    pGraphics.reset();
}