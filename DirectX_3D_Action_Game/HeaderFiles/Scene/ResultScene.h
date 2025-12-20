#pragma once
#include "BaseScene.h"

class ResultScene : public BaseScene {
public:
    using BaseScene::BaseScene;

    static bool isClear;

    void Initialize() override;
    void Update(float dt) override;
    void Draw() override;
};