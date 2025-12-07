/*===================================================================
//ファイル:Game.cpp
//概要:Gameクラスの実装
=====================================================================*/
#include "App/Game.h"
#include "App/Main.h"
#include "Scene/GameScene.h" // 最初のシーン

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

    // 最初のシーンへ遷移 (GameScene)
    pSceneManager->ChangeScene<GameScene>();

    return true;
}

void Game::Update(float dt) {
    pInput->Update();

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

    // 画面フリップ
    pGraphics->EndFrame();
}

void Game::Shutdown() {
    pSceneManager.reset();
    pGraphics.reset();
}