/*===================================================================
//ファイル:Game.h
//概要:アプリケーション全体を管理する中核クラス
//     GraphicsやSceneManagerの寿命を管理する
=====================================================================*/
#pragma once
#include <Windows.h>
#include <memory>
#include "Engine/Graphics.h"
#include "Engine/Input.h"
#include "Engine/Audio.h"
#include "Scene/SceneManager.h"
#include "ECS/Components/PlayerComponent.h" // PlayerTypeの定義用

class Game {
public:
    Game();
    ~Game();

    // 初期化
    bool Initialize(HWND hWnd);
    // 更新
    void Update(float dt);
    // 描画
    void Draw();
    // 終了処理
    void Shutdown();

    // グローバルアクセサ
    static Game* GetInstance() { return instance; }

    // ゲッター
    Graphics* GetGraphics() const { return pGraphics.get(); }
    Input* GetInput() const { return pInput.get(); }
    Audio* GetAudio() const { return pAudio.get(); }
    SceneManager* GetSceneManager() const { return pSceneManager.get(); }
    HWND GetWindowHandle() const { return m_hWnd; }

    // --- ゲーム進行データ管理 ---

    // プレイヤータイプ
    void SetPlayerType(PlayerType type) { m_selectedPlayerType = type; }
    PlayerType GetPlayerType() const { return m_selectedPlayerType; }

    // ステージ管理
    void SetCurrentStage(int stage) { m_currentStage = stage; }
    int GetCurrentStage() const { return m_currentStage; }
    // void NextStage() { m_currentStage++; } // 新しいロジックでは手動で管理するので不要ですが、残しても害はありません

    // フェーズ管理 (新規追加)
    void SetCurrentPhase(int phase) { m_currentPhase = phase; }
    int GetCurrentPhase() const { return m_currentPhase; }
    void NextPhase() { m_currentPhase++; }

    // ステージ解放状況 (新規追加)
    int GetMaxUnlockedStage() const { return m_maxUnlockedStage; }
    void UnlockNextStage() {
        if (m_currentStage >= m_maxUnlockedStage && m_maxUnlockedStage < 5) {
            m_maxUnlockedStage++;
        }
    }

    // HP引き継ぎ用 (新規追加)
    void SetSavedHP(int hp) { m_savedPlayerHP = hp; }
    int GetSavedHP() const { return m_savedPlayerHP; }

private:
    static Game* instance; // シングルトンインスタンス
    HWND m_hWnd = nullptr; // ウィンドウハンドル

    // サブシステム
    std::unique_ptr<Graphics> pGraphics;
    std::unique_ptr<Input> pInput;
    std::unique_ptr<Audio> pAudio;
    std::unique_ptr<SceneManager> pSceneManager;

    // ゲームデータ
    PlayerType m_selectedPlayerType = PlayerType::AssaultStriker;

    int m_currentStage = 1;       // 現在のステージ
    int m_currentPhase = 1;       // 現在のフェーズ
    int m_maxUnlockedStage = 1;   // 解放済みステージ
    int m_savedPlayerHP = -1;     // フェーズ間のHP引き継ぎ (-1で全快)
};