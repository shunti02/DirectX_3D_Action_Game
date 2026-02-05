#pragma once
#include "Scene/BaseScene.h"
#include "Scene/SceneManager.h"
#include "App/Game.h"
#include "../ImGui/imgui.h" 
#include "Engine/SkyBox.h"

// 前方宣言
class PlayerAnimationSystem;

class CharacterSelectScene : public BaseScene {
public:
    CharacterSelectScene(SceneManager* manager);
    ~CharacterSelectScene() override = default;
    void Initialize() override;
    void Update(float dt) override;
    void Draw() override;
    void Shutdown() override;

private:
    void UpdatePreviewModel();
   
    int m_currentSelectIndex = 0;
    std::unique_ptr<SkyBox> pSkyBox;

    // 現在表示中のモデルID (1つだけ)
    int previewModelID = -1;

    PlayerAnimationSystem* m_pAnimSystem = nullptr;

    // UIボタンの当たり判定用
    const float ARROW_Y = 360.0f; // 矢印の高さ
    const float ARROW_SIZE = 80.0f; // 矢印のサイズ

};