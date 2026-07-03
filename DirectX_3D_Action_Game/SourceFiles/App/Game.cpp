#include "App/Game.h"
#include "App/Main.h"
#include "Scene/GameScene.h"
#include "Scene/TitleScene.h"
#include "../ImGui/imgui.h"
#include "Engine/AnimationManager.h"
#include <fstream>
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
        pAudio->LoadSound("BGM_CUSTOMIZE", L"Assets/Audio/customize_bgm.wav");
        pAudio->LoadSound("BGM_SELECT", L"Assets/Audio/select_bgm.wav");
        pAudio->LoadSound("BGM_GAME", L"Assets/Audio/game_bgm.wav");
        pAudio->LoadSound("BGM_RESULT", L"Assets/Audio/result_bgm.wav");
    }
    bool isAnimLoaded = AnimationManager::GetInstance()->LoadAnimation("AttackRight", "Assets/Animations/AttackRight.json");
    if (isAnimLoaded) {
        AppLog::AddLog("[Animation] AttackRight.json Loaded Successfully.");
    }
    else {
        AppLog::AddLog("[ERROR] Failed to load AttackRight.json!");
        MessageBoxA(m_hWnd,
            "AttackRight.json の読み込みに失敗しました。\nファイルの配置場所、またはファイル名（拡張子）を確認してください。",
            "ファイル読み込みエラー",
            MB_OK | MB_ICONERROR);
    }

    bool isAnimLoadedLeft = AnimationManager::GetInstance()->LoadAnimation("AttackLeft", "Assets/Animations/AttackLeft.json");
    if (isAnimLoadedLeft) {
        AppLog::AddLog("[Animation] AttackLeft.json Loaded Successfully.");
    }
    else {
        AppLog::AddLog("[ERROR] Failed to load AttackLeft.json!");
        MessageBoxA(m_hWnd, "AttackLeft.json の読み込みに失敗しました。", "ファイル読み込みエラー", MB_OK | MB_ICONERROR);
    }

    bool isShootRightLoaded = AnimationManager::GetInstance()->LoadAnimation("ShootRight", "Assets/Animations/ShootRight.json");
    if (!isShootRightLoaded) {
        AppLog::AddLog("[ERROR] Failed to load ShootRight.json!");
        MessageBoxA(m_hWnd, "ShootRight.json の読み込みに失敗しました。", "エラー", MB_OK | MB_ICONERROR);
    }

    bool isShootLeftLoaded = AnimationManager::GetInstance()->LoadAnimation("ShootLeft", "Assets/Animations/ShootLeft.json");
    if (!isShootLeftLoaded) {
        AppLog::AddLog("[ERROR] Failed to load ShootLeft.json!");
        MessageBoxA(m_hWnd, "ShootLeft.json の読み込みに失敗しました。", "エラー", MB_OK | MB_ICONERROR);
    }

    bool isIdleLoaded = AnimationManager::GetInstance()->LoadAnimation("Idle", "Assets/Animations/Idle.json");
    if (!isIdleLoaded) { AppLog::AddLog("[ERROR] Failed to load Idle.json!"); MessageBoxA(m_hWnd, "Idle.json の読み込みに失敗しました。", "エラー", MB_OK | MB_ICONERROR); }

    bool isMoveLoaded = AnimationManager::GetInstance()->LoadAnimation("Move", "Assets/Animations/Move.json");
    if (!isMoveLoaded) { AppLog::AddLog("[ERROR] Failed to load Move.json!"); MessageBoxA(m_hWnd, "Move.json の読み込みに失敗しました。", "エラー", MB_OK | MB_ICONERROR); }

    bool isJU = AnimationManager::GetInstance()->LoadAnimation("JumpUp", "Assets/Animations/JumpUp.json");
    if (!isJU) { MessageBoxA(m_hWnd, "JumpUp.json の読み込みに失敗しました。", "エラー", MB_OK); }

    bool isJD = AnimationManager::GetInstance()->LoadAnimation("JumpDown", "Assets/Animations/JumpDown.json");
    if (!isJD) { MessageBoxA(m_hWnd, "JumpDown.json の読み込みに失敗しました。", "エラー", MB_OK); }

    bool isHurt = AnimationManager::GetInstance()->LoadAnimation("Hurt", "Assets/Animations/Hurt.json");
    if (!isHurt) { MessageBoxA(m_hWnd, "Hurt.json の読み込みに失敗しました。", "エラー", MB_OK); }

    bool isDeadLoaded = AnimationManager::GetInstance()->LoadAnimation("Dead", "Assets/Animations/Dead.json");
    if (!isDeadLoaded) {
        AppLog::AddLog("[ERROR] Failed to load Dead.json!");
        MessageBoxA(m_hWnd, "Dead.json の読み込みに失敗しました。", "エラー", MB_OK | MB_ICONERROR);
    }
    //シーン初期化
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

//セーブ
void Game::SaveGame() {
    std::ofstream file("savedata.txt");
    if (file.is_open()) {
        file << m_maxUnlockedStage << "\n"
            << m_customizeData.headID << "\n"
            << m_customizeData.bodyID << "\n"
            << m_customizeData.armLeftID << "\n"
            << m_customizeData.armRightID << "\n"
            << m_customizeData.legID << "\n";

        file.close();
        AppLog::AddLog("[System] Game Saved.");
    }
}

//ロード
bool Game::LoadGame() {
    std::ifstream file("savedata.txt");
    if (file.is_open()) {
        file >> m_maxUnlockedStage
            >> m_customizeData.headID
            >> m_customizeData.bodyID
            >> m_customizeData.armLeftID
            >> m_customizeData.armRightID
            >> m_customizeData.legID;

        if (m_maxUnlockedStage < 1) m_maxUnlockedStage = 1;
        if (m_maxUnlockedStage > 3) m_maxUnlockedStage = 3;

        file.close();
        AppLog::AddLog("[System] Game Loaded.");
        return true;
    }
    return false;
}

//リセット
void Game::ResetGame() {
    m_maxUnlockedStage = 1;
    m_currentStage = 1;
    m_currentPhase = 1;
    m_savedPlayerHP = -1;

    m_selectedPlayerType = PlayerType::AssaultStriker;

    SaveGame();
}