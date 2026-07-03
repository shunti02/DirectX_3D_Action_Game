#pragma once
#include <memory>
#include <functional>
#include "BaseScene.h"

enum class FadeState {
    None, 
    FadeOut,
    FadeIn
};

class SceneManager {
public:
    SceneManager();
    ~SceneManager();

    void Update(float dt);
    void Draw();

    template <typename T, typename... Args>
    void ChangeScene(Args&&... args) {
        if (m_fadeState != FadeState::None) return;

        m_fadeState = FadeState::FadeOut;

        m_nextSceneCreator = [this, args...]() {
            if (currentScene) {
                currentScene->Shutdown();
            }
            currentScene = std::make_unique<T>(this, args...);
            currentScene->Initialize();
            };
    }

private:
    std::unique_ptr<BaseScene> currentScene;

    std::function<void()> m_nextSceneCreator = nullptr;

    FadeState m_fadeState = FadeState::None;
    float m_fadeAlpha = 0.0f;
    float m_fadeSpeed = 2.0f;
};
