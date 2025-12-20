/*===================================================================
// ファイル: GameScene.h
// 概要: ゲームプレイ中のシーン（クラス宣言）
=====================================================================*/
#pragma once
#include "BaseScene.h"

class GameScene : public BaseScene {
public:
    using BaseScene::BaseScene; // コンストラクタ継承

    void Initialize() override;
    void Update(float dt) override;

private:
    // 勝敗判定を行う内部関数
    void CheckGameCondition();
};
