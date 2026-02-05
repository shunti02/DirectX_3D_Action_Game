///*===================================================================
////ファイル:SceneManager.h
////概要:シーンの切り替えを管理するクラス
//=====================================================================*/
//#pragma once
//#include <memory>
//#include <functional>
//#include "BaseScene.h"
//
//// フェードの状態
//enum class FadeState {
//    None,       // 通常
//    FadeOut,    // 暗くなっていく
//    FadeIn      // 明るくなっていく
//};
//class SceneManager {
//public:
//    SceneManager() = default;
//    ~SceneManager() {
//        if (currentScene) {
//            currentScene->Shutdown();
//            currentScene.reset();
//        }
//    }
//
//    void Update(float dt) {
//        // 次のシーンが予約されていたら切り替え実行
//        if (nextScene) {
//            if (currentScene) {
//                currentScene->Shutdown();
//            }
//            currentScene = std::move(nextScene);
//            currentScene->Initialize();
//        }
//
//        if (currentScene) {
//            currentScene->Update(dt);
//        }
//    }
//
//    void Draw() {
//        if (currentScene) {
//            currentScene->Draw();
//        }
//    }
//
//    // シーン変更リクエスト（テンプレートで型安全に生成）
//    // 使用例: ChangeScene<GameScene>();
//    template <typename T, typename... Args>
//    void ChangeScene(Args&&... args) {
//        // 即座に変えず、次のフレームの冒頭で変えるために予約する
//        nextScene = std::make_unique<T>(this, std::forward<Args>(args)...);
//    }
//
//private:
//    std::unique_ptr<BaseScene> currentScene;
//    std::unique_ptr<BaseScene> nextScene;
//};
/*===================================================================
//ファイル:SceneManager.h
//概要:シーンの切り替えとフェードを管理するクラス
=====================================================================*/
#pragma once
#include <memory>
#include <functional>
#include "BaseScene.h"

// フェードの状態
enum class FadeState {
    None,       // 通常
    FadeOut,    // 暗くなっていく
    FadeIn      // 明るくなっていく
};

class SceneManager {
public:
    SceneManager();
    ~SceneManager();

    void Update(float dt);
    void Draw();

    // シーン変更リクエスト
    // テンプレート関数はヘッダーに実装を書く必要があります
    template <typename T, typename... Args>
    void ChangeScene(Args&&... args) {
        // フェード中なら受け付けない
        if (m_fadeState != FadeState::None) return;

        // フェードアウト開始
        m_fadeState = FadeState::FadeOut;

        // 次のシーンを生成する処理を「予約」しておく (まだ実行しない)
        // ラムダ式の中に、引数(args)も含めて保存します
        m_nextSceneCreator = [this, args...]() {
            if (currentScene) {
                currentScene->Shutdown();
            }
            // 新しいシーンを作成して初期化
            currentScene = std::make_unique<T>(this, args...);
            currentScene->Initialize();
            };
    }

private:
    std::unique_ptr<BaseScene> currentScene;

    // ★変更: nextSceneポインタの代わりに、生成関数を保存する変数にします
    // (フェードアウトが終わった瞬間に実行するため)
    std::function<void()> m_nextSceneCreator = nullptr;

    // ★追加: フェード用変数
    FadeState m_fadeState = FadeState::None;
    float m_fadeAlpha = 0.0f;   // 0.0(透明) ~ 1.0(真っ黒)
    float m_fadeSpeed = 2.0f;   // フェード速度
};
