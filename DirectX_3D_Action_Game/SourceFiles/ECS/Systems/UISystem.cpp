/*===================================================================
// ファイル: UISystem.cpp
// 概要: ImGuiを使用してプレイヤーや敵のHPを可視化する
=====================================================================*/
#define NOMINMAX
#include "ECS/Systems/UISystem.h"
#include "ECS/World.h"
#include "ECS/Components/StatusComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/EnemyComponent.h"
#include "ECS/Components/RolesComponent.h"
#include "../../../ImGui/imgui.h"
#include "App/Main.h"
#include <format>
#include <string>

void UISystem::Update(float dt) {
    auto registry = pWorld->GetRegistry();
    // -----------------------------------------------------
    // 1. ステータスモニター
    // -----------------------------------------------------
    // ImGuiウィンドウの開始
    ImGui::Begin("Game Status Monitor");

    // --- プレイヤー情報の表示 ---
    if (ImGui::CollapsingHeader("Players", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
            if (!registry->HasComponent<PlayerComponent>(id)) continue;
            if (!registry->HasComponent<StatusComponent>(id)) continue;

            auto& status = registry->GetComponent<StatusComponent>(id);
            auto& player = registry->GetComponent<PlayerComponent>(id);

            std::string roleName = "Unknown";
            if (registry->HasComponent<AttackerTag>(id)) roleName = "Attacker (Blue)";
            if (registry->HasComponent<HealerTag>(id)) roleName = "Healer (Green)";

            std::string label = std::format("Player ID:{} [{}]", id, roleName);
            if (player.isActive) label += " <RENI CONTROL>"; // 操作中キャラ

            ImGui::Text("%s", label.c_str());

            // HPバーの表示 (現在HP / 最大HP)
            float hpFraction = (float)status.hp / (float)status.maxHp;
            // HPに応じて色を変える (緑 -> 黄 -> 赤)
            if (hpFraction > 0.5f) ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
            else if (hpFraction > 0.2f) ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
            else ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));

            std::string hpText = std::format("HP: {}/{}", status.hp, status.maxHp);
            ImGui::ProgressBar(hpFraction, ImVec2(200.0f, 0.0f), hpText.c_str());

            ImGui::PopStyleColor();
            ImGui::Separator();
        }
    }

    // --- 敵情報の表示 ---
    if (ImGui::CollapsingHeader("Enemies", ImGuiTreeNodeFlags_DefaultOpen)) {
        int enemyCount = 0;
        for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
            // EnemyComponentまたはStatusを持つ敵とみなせるものを表示
            // ※EnemyComponentの実装有無によるので、一旦StatusとColliderを持つものを敵候補として表示
            if (registry->HasComponent<PlayerComponent>(id)) continue; // プレイヤーは除外
            if (!registry->HasComponent<StatusComponent>(id)) continue;

            enemyCount++;
            auto& status = registry->GetComponent<StatusComponent>(id);

            std::string label = std::format("Enemy ID:{}", id);
            ImGui::Text("%s", label.c_str());

            float hpFraction = (float)status.hp / (float)status.maxHp;
            std::string hpText = std::format("{}/{}", status.hp, status.maxHp);

            // 敵は赤色バー
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
            ImGui::ProgressBar(hpFraction, ImVec2(150.0f, 0.0f), hpText.c_str());
            ImGui::PopStyleColor();
        }

        if (enemyCount == 0) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "All Enemies Defeated!");
        }
    }

    ImGui::End();

    // -----------------------------------------------------
    // 2. ★追加: デバッグログウィンドウ
    // -----------------------------------------------------
    ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin("Debug Log");

    if (ImGui::Button("Clear Log")) {
        AppLog::Clear();
    }
    ImGui::SameLine();
    ImGui::Text("History: %d", AppLog::logs.size());

    ImGui::Separator();

    // スクロール領域
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    for (const auto& log : AppLog::logs) {
        ImGui::TextUnformatted(log.c_str());
    }

    // 常に最新（一番下）を表示する
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
    ImGui::End(); // Debug Log 終了
}