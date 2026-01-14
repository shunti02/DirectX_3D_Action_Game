/*===================================================================
// ファイル: GameScene.h
// 概要: ゲームプレイ中のシーン（クラス宣言）
=====================================================================*/
#pragma once
#include "BaseScene.h"
#include "Engine/SkyBox.h" 

// 前方宣言
class UISystem;

class GameScene : public BaseScene {
public:
    using BaseScene::BaseScene; // コンストラクタ継承

    void Initialize() override;
    void Update(float dt) override;
    void Draw() override;
private:
    // 勝敗判定を行う内部関数
    void CheckGameCondition();
    std::unique_ptr<SkyBox> pSkyBox;
    UISystem* pUISystem = nullptr;
};
