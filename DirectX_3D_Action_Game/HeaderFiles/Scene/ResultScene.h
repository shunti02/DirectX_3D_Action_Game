#pragma once
#include "BaseScene.h"
#include "SceneManager.h" // コンストラクタで必要

class ResultScene : public BaseScene {
public:
    // コンストラクタ追加
    ResultScene(SceneManager* manager);

    // クリアフラグ (staticのまま維持)
    static bool isClear;

    void Initialize() override;
    void Update(float dt) override;
    void Draw() override;
    void Shutdown() override;

private:
    // 現在選択中の項目 (0:Retry, 1:Customize, 2:Title)
    int m_selectIndex = 0;
};