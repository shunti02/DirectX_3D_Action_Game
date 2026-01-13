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

    // グローバルアクセサ（どこからでもGameインスタンスにアクセス可能にする）
    static Game* GetInstance() { return instance; }
    Graphics* GetGraphics() const { return pGraphics.get(); }
    Input* GetInput() const { return pInput.get(); }
    Audio* GetAudio() const { return pAudio.get(); }
    SceneManager* GetSceneManager() const { return pSceneManager.get(); }

private:
    static Game* instance; // シングルトンインスタンス

    std::unique_ptr<Graphics> pGraphics;
    std::unique_ptr<Input> pInput;
    std::unique_ptr<Audio> pAudio;
    std::unique_ptr<SceneManager> pSceneManager;
};
