#pragma once
#include "BaseScene.h"
#include "SceneManager.h"
#include "App/Game.h"
#include "Engine/SkyBox.h"

class StageSelectScene : public BaseScene {
public:
    StageSelectScene(SceneManager* manager);
    void Initialize() override;
    void Update(float dt) override;
    void Draw() override;
    void Shutdown() override;

private:
    std::unique_ptr<SkyBox> pSkyBox;
    int m_selectIndex = 0; // 0~4 (Stage 1~5)
};
