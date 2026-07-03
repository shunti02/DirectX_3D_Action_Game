#pragma once
#include "BaseScene.h"
#include "Engine/SkyBox.h" 
#include "ECS/Systems/UISystem.h"
class UISystem;
class RenderSystem;
class CameraSystem;
class PlayerAnimationSystem;
class EnemyAnimationSystem;

class GameScene : public BaseScene {
public:
    using BaseScene::BaseScene;

    void Initialize() override;
    void Update(float dt) override;
    void Draw() override;
    void Shutdown() override;
private:
    void CheckGameCondition();
    std::unique_ptr<SkyBox> pSkyBox;
    UISystem* pUISystem = nullptr;
    float m_startTimer = 0.0f;
    const float START_WAIT_TIME = 4.0f;
    RenderSystem* m_pRenderSystem = nullptr;
    CameraSystem* m_pCameraSystem = nullptr;
    PlayerAnimationSystem* m_pAnimSystem = nullptr;
    EnemyAnimationSystem* m_pEnemyAnimSystem = nullptr;
    bool m_isSceneChanging = false;
    bool m_isGameOverSequence = false;
    float m_gameOverTimer = 0.0f;
};
