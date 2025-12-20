#pragma once
#include "BaseScene.h"

class TitleScene : public BaseScene {
public:
    using BaseScene::BaseScene;

    void Initialize() override;
    void Update(float dt) override;
    void Draw() override;
};