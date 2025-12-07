/*===================================================================
//ファイル:SceneManager.h
//概要:シーンの切り替えを管理するクラス
=====================================================================*/
#pragma once
#include <memory>
#include "BaseScene.h"

class SceneManager {
public:
    SceneManager() = default;
    ~SceneManager() {
        if (currentScene) {
            currentScene->Shutdown();
            currentScene.reset();
        }
    }

    void Update(float dt) {
        // 次のシーンが予約されていたら切り替え実行
        if (nextScene) {
            if (currentScene) {
                currentScene->Shutdown();
            }
            currentScene = std::move(nextScene);
            currentScene->Initialize();
        }

        if (currentScene) {
            currentScene->Update(dt);
        }
    }

    void Draw() {
        if (currentScene) {
            currentScene->Draw();
        }
    }

    // シーン変更リクエスト（テンプレートで型安全に生成）
    // 使用例: ChangeScene<GameScene>();
    template <typename T, typename... Args>
    void ChangeScene(Args&&... args) {
        // 即座に変えず、次のフレームの冒頭で変えるために予約する
        nextScene = std::make_unique<T>(this, std::forward<Args>(args)...);
    }

private:
    std::unique_ptr<BaseScene> currentScene;
    std::unique_ptr<BaseScene> nextScene;
};
