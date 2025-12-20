#include "Scene/ResultScene.h"
#include "App/Game.h"
#include "Scene/TitleScene.h"
#include "App/Main.h"
#include "../ImGui/imgui.h"

bool ResultScene::isClear = false;

void ResultScene::Initialize() {}

void ResultScene::Update(float dt) {
    if (Game::GetInstance()->GetInput()->IsKeyDown(VK_RETURN)) {
        Game::GetInstance()->GetSceneManager()->ChangeScene<TitleScene>();
    }
}

void ResultScene::Draw() {
    ImGui::SetNextWindowPos(ImVec2(Config::SCREEN_WIDTH / 2 - 150, Config::SCREEN_HEIGHT / 2 - 50));
    ImGui::SetNextWindowSize(ImVec2(300, 150));
    ImGui::Begin("Result", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground);

    ImGui::SetWindowFontScale(3.0f);
    if (isClear) {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "GAME CLEAR!!");
    }
    else {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "GAME OVER...");
    }
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Text("Press ENTER to Title");

    ImGui::End();
}