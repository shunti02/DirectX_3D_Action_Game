/*===================================================================
//ファイル:SceneManager.cpp
//概要:シーン管理の実装
=====================================================================*/
#include "Scene/SceneManager.h"
#include "App/Game.h"
#include "App/Main.h" // Config::SCREEN_WIDTH等のため

SceneManager::SceneManager() {
    m_fadeState = FadeState::None;
    m_fadeAlpha = 0.0f;
}

SceneManager::~SceneManager() {
    if (currentScene) {
        currentScene->Shutdown();
        currentScene.reset();
    }
}

void SceneManager::Update(float dt) {
    // 現在のシーンの更新
    if (currentScene) {
        currentScene->Update(dt);
    }

    // --------------------------------------------------
    // フェード処理
    // --------------------------------------------------
    if (m_fadeState == FadeState::FadeOut) {
        // 暗くしていく (Alphaを増やす)
        m_fadeAlpha += m_fadeSpeed * dt;
        if (m_fadeAlpha >= 1.0f) {
            m_fadeAlpha = 1.0f;

            // --- 画面が真っ暗になったタイミングでシーンを切り替える ---
            if (m_nextSceneCreator) {
                m_nextSceneCreator(); // 予約しておいた生成関数を実行
                m_nextSceneCreator = nullptr; // 使い終わったら空にする
            }

            // フェードインへ移行
            m_fadeState = FadeState::FadeIn;
        }
    }
    else if (m_fadeState == FadeState::FadeIn) {
        // 明るくしていく (Alphaを減らす)
        m_fadeAlpha -= m_fadeSpeed * dt;
        if (m_fadeAlpha <= 0.0f) {
            m_fadeAlpha = 0.0f;
            m_fadeState = FadeState::None; // フェード完了
        }
    }
}

void SceneManager::Draw() {
    // 1. シーンの描画
    if (currentScene) {
        currentScene->Draw();
    }

    // 2. フェード用の黒画像を描画 (一番手前に上書き)
    if (m_fadeAlpha > 0.0f) {
        Graphics* g = Game::GetInstance()->GetGraphics();
        if (g) {
            g->BeginDraw2D();

            // アルファ値に応じた黒色を作成 (A, R, G, B)
            // 0xFF000000 などの形式にする
            uint32_t alpha = static_cast<uint32_t>(m_fadeAlpha * 255.0f);
            uint32_t color = (alpha << 24) | 0x000000; // 黒

            // 画面全体を覆う四角形
            float w = (float)Config::SCREEN_WIDTH;
            float h = (float)Config::SCREEN_HEIGHT;
            g->FillRect(0, 0, w, h, color);

            g->EndDraw2D();
        }
    }
}