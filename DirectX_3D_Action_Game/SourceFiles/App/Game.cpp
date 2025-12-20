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

    // 最初のシーンへ遷移 (GameScene)
    pSceneManager->ChangeScene<TitleScene>();

    return true;
}

void Game::Update(float dt) {
    pInput->Update();

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
    pGraphics.reset();
}