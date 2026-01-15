#pragma once
#include "Scene/BaseScene.h"
#include "Scene/SceneManager.h"
#include "App/Game.h"
#include "../ImGui/imgui.h" 
#include "Engine/SkyBox.h"

class CharacterSelectScene : public BaseScene {
public:
    CharacterSelectScene(SceneManager* manager);

    void Initialize() override;
    void Update(float dt) override;
    void Draw() override;
    void Shutdown() override;

private:
    void UpdatePreviewModel();

    int m_currentSelectIndex = 0;
    std::unique_ptr<SkyBox> pSkyBox;
    int previewModelID = -1; // •\Ž¦’†‚Ìƒ‚ƒfƒ‹ID
};