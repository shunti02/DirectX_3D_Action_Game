/*===================================================================
// ファイル: GameScene.h
// 概要: ゲームプレイ中のシーン（クラス宣言）
=====================================================================*/
#pragma once
#include "BaseScene.h"
#include "Engine/SkyBox.h" 
#include "ECS/Systems/UISystem.h"
// 前方宣言
class UISystem;
class RenderSystem;
class CameraSystem;
class PlayerAnimationSystem;
class EnemyAnimationSystem;

class GameScene : public BaseScene {
public:
    using BaseScene::BaseScene; // コンストラクタ継承

    void Initialize() override;
    void Update(float dt) override;
    void Draw() override;
    void Shutdown() override;
private:
    // 勝敗判定を行う内部関数
    void CheckGameCondition();
    std::unique_ptr<SkyBox> pSkyBox;
    UISystem* pUISystem = nullptr;

    // ★追加: 開始演出用
    float m_startTimer = 0.0f; // 経過時間
    const float START_WAIT_TIME = 4.0f; // 操作不能時間 (3秒Ready + 1秒GO)

    // ★追加: 演出中も動かしたいシステムへのポインタ
    RenderSystem* m_pRenderSystem = nullptr;
    CameraSystem* m_pCameraSystem = nullptr;
    PlayerAnimationSystem* m_pAnimSystem = nullptr;
    EnemyAnimationSystem* m_pEnemyAnimSystem = nullptr;
    // ★追加: シーン遷移中かどうかを管理するフラグ
    bool m_isSceneChanging = false;
};
