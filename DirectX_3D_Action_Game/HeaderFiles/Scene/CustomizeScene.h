#pragma once
#include "Scene/BaseScene.h"
#include "Scene/SceneManager.h"
#include "App/Game.h"
#include "Engine/SkyBox.h"

class PlayerAnimationSystem;

class CustomizeScene : public BaseScene {
public:
    CustomizeScene(SceneManager* manager);
    ~CustomizeScene() override = default;

    void Initialize() override;
    void Update(float dt) override;
    void Draw() override;
    void Shutdown() override;

private:
    void UpdatePreviewMesh();
    int m_selectedCategory = 0;

    CustomizeData m_currentConfig;

    std::unique_ptr<SkyBox> pSkyBox;
    int previewModelID = -1;
    PlayerAnimationSystem* m_pAnimSystem = nullptr;

    float m_blinkTimer = 0.0f;
};