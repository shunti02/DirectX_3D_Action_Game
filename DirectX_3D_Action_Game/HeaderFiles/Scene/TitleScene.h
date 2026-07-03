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
    void Shutdown() override;
private:
    std::unique_ptr<SkyBox> pSkyBox;
    std::unique_ptr<UISystem> pUISystem; 

    float cameraAngle = 0.0f;

    float blinkTimer = 0.0f;
    int m_selectIndex = 0;
};