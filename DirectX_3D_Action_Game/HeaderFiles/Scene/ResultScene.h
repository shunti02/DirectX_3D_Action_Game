#pragma once
#include "BaseScene.h"
#include "SceneManager.h"

class ResultScene : public BaseScene {
public:
    ResultScene(SceneManager* manager);

    static bool isClear;

    void Initialize() override;
    void Update(float dt) override;
    void Draw() override;
    void Shutdown() override;

private:
    int m_selectIndex = 0;
};