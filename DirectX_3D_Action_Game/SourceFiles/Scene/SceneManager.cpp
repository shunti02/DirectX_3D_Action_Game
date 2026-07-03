#include "Scene/SceneManager.h"
#include "App/Game.h"
#include "App/Main.h"

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
    if (currentScene) {
        currentScene->Update(dt);
    }
    //フェード
    if (m_fadeState == FadeState::FadeOut) {
        m_fadeAlpha += m_fadeSpeed * dt;
        if (m_fadeAlpha >= 1.0f) {
            m_fadeAlpha = 1.0f;

            if (m_nextSceneCreator) {
                m_nextSceneCreator();
                m_nextSceneCreator = nullptr;
            }
            m_fadeState = FadeState::FadeIn;
        }
    }
    else if (m_fadeState == FadeState::FadeIn) {
        m_fadeAlpha -= m_fadeSpeed * dt;
        if (m_fadeAlpha <= 0.0f) {
            m_fadeAlpha = 0.0f;
            m_fadeState = FadeState::None;
        }
    }
}

void SceneManager::Draw() {
    if (currentScene) {
        currentScene->Draw();
    }

    if (m_fadeAlpha > 0.0f) {
        Graphics* g = Game::GetInstance()->GetGraphics();
        if (g) {
            g->BeginDraw2D();
            uint32_t alpha = static_cast<uint32_t>(m_fadeAlpha * 255.0f);
            uint32_t color = (alpha << 24) | 0x000000;

            float w = (float)Config::SCREEN_WIDTH;
            float h = (float)Config::SCREEN_HEIGHT;
            g->FillRect(0, 0, w, h, color);

            g->EndDraw2D();
        }
    }
}