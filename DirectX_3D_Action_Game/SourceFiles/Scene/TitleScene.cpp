#include "Scene/TitleScene.h"
#include "App/Game.h"
#include "Scene/GameScene.h"
#include "App/Main.h"
#include "../ImGui/imgui.h"

void TitleScene::Initialize() {}

void TitleScene::Update(float dt) {
    if (Game::GetInstance()->GetInput()->IsKeyDown(VK_RETURN)) {
        Game::GetInstance()->GetSceneManager()->ChangeScene<GameScene>();
    }
}

void TitleScene::Draw() {
    ImGui::SetNextWindowPos(ImVec2(Config::SCREEN_WIDTH / 2 - 100, Config::SCREEN_HEIGHT / 2 - 50));
    ImGui::SetNextWindowSize(ImVec2(200, 100));
    ImGui::Begin("Title", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground);

    ImGui::SetWindowFontScale(2.0f);
    ImGui::Text("ACTION GAME");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Text("Press ENTER to Start");

    ImGui::End();
}