/*===================================================================
// ファイル: TitleScene.h
=====================================================================*/
#pragma once
#include "BaseScene.h"
#include "SceneManager.h"
#include "Engine/SkyBox.h"
#include "ECS/Systems/UISystem.h"

class TitleScene : public BaseScene {
public:
    using BaseScene::BaseScene;

    void Initialize() override;
    void Update(float dt) override;
    void Draw() override;
private:
    std::unique_ptr<SkyBox> pSkyBox;
    std::unique_ptr<UISystem> pUISystem; // 文字描画用

    // 背景回転演出用のカメラ角度
    float cameraAngle = 0.0f;

    // 文字の点滅用タイマー
    float blinkTimer = 0.0f;
    // ★追加: 選択中のメニュー (0: NEW GAME, 1: CONTINUE)
    int m_selectIndex = 0;
};