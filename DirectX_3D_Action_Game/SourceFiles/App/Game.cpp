/*===================================================================
//ファイル:Game.cpp
//概要:Gameクラスの実装
=====================================================================*/
#include "App/Game.h"
#include "App/Main.h"
#include "Scene/GameScene.h"
#include "Scene/TitleScene.h"
#include "../ImGui/imgui.h"
#include <fstream> // ファイル読み書き用
#include <string>

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
    m_hWnd = hWnd;
    pInput->Initialize(hWnd);
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
    pInput->ResetMouseWheel();
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

// ★追加: セーブ機能
void Game::SaveGame() {
    std::ofstream file("savedata.txt");
    if (file.is_open()) {
        // 解放済みステージ数と、現在選択中のプレイヤータイプを保存
        // (スペース区切りで書き込む)
        file << m_maxUnlockedStage << " " << (int)m_selectedPlayerType;

        file.close();
        AppLog::AddLog("[System] Game Saved. MaxStage:%d, Type:%d", m_maxUnlockedStage, (int)m_selectedPlayerType);
    }
}

// ★追加: ロード機能
bool Game::LoadGame() {
    std::ifstream file("savedata.txt");
    if (file.is_open()) {
        int loadedType = 0;

        // 解放済みステージ数とプレイヤータイプを読み込む
        file >> m_maxUnlockedStage >> loadedType;

        // 安全対策: 範囲チェック
        if (m_maxUnlockedStage < 1) m_maxUnlockedStage = 1;
        if (m_maxUnlockedStage > 5) m_maxUnlockedStage = 5;

        // プレイヤータイプの反映
        // (Enumの範囲内かチェック)
        if (loadedType >= (int)PlayerType::AssaultStriker && loadedType <= (int)PlayerType::PlasmaSniper) {
            m_selectedPlayerType = (PlayerType)loadedType;
        }
        else {
            m_selectedPlayerType = PlayerType::AssaultStriker; // デフォルト
        }

        file.close();
        AppLog::AddLog("[System] Game Loaded. MaxStage:%d, Type:%d", m_maxUnlockedStage, (int)m_selectedPlayerType);
        return true; // ロード成功
    }
    return false; // ファイルがない（初回プレイなど）
}

// ★追加: リセット機能 (NEW GAME)
void Game::ResetGame() {
    m_maxUnlockedStage = 1;
    m_currentStage = 1;
    m_currentPhase = 1;
    m_savedPlayerHP = -1;

    // NEW GAME時はプレイヤータイプはデフォルトに戻さない方が親切な場合もありますが、
    // ここでは完全に初期化するならデフォルトに戻します。
    // (もしキャラ選択画面から始まるなら、そこで上書きされるので気にしなくてOK)
    m_selectedPlayerType = PlayerType::AssaultStriker;

    SaveGame(); // リセットした状態を保存
}