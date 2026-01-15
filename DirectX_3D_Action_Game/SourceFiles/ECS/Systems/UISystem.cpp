/*===================================================================
// ファイル: UISystem.cpp
// 概要: UI描画システム (ImGui + Direct2D)
=====================================================================*/
#define NOMINMAX
#include "ECS/Systems/UISystem.h"
#include "ECS/World.h"
#include "ECS/Components/StatusComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/EnemyComponent.h"
#include "ECS/Components/RolesComponent.h"
#include "../../../ImGui/imgui.h"
#include "App/Game.h"
#include "Engine/Graphics.h"
#include "App/Main.h"
#include <format>
#include <string>

// ワールド情報を保存
void UISystem::Init(World* world) {
    pWorld = world;
}
// 文字列変換用ヘルパー (std::string -> std::wstring)
std::wstring ToWString(const std::string& str) {
    size_t newsize = str.length() + 1;
    std::wstring wc(newsize, L'\0');
    size_t convertedChars = 0;
    mbstowcs_s(&convertedChars, &wc[0], newsize, str.c_str(), _TRUNCATE);
    return wc;
}

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

void UISystem::Draw(Graphics* pGraphics) {
    if (!pGraphics) return;

    // プレイヤーと敵の情報を収集
    auto registry = pWorld->GetRegistry();
    EntityID playerID = ECSConfig::INVALID_ID;
    int enemyCount = 0;

    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (registry->HasComponent<PlayerComponent>(id)) {
            if (registry->GetComponent<PlayerComponent>(id).isActive) {
                playerID = id;
            }
        }
        else if (registry->HasComponent<EnemyComponent>(id) &&
            registry->HasComponent<StatusComponent>(id))
        {
            if (registry->GetComponent<StatusComponent>(id).hp > 0) {
                enemyCount++;
            }
        }
    }

    // Direct2D 描画開始
    pGraphics->BeginDraw2D();

    float scW = (float)Config::SCREEN_WIDTH;
    float scH = (float)Config::SCREEN_HEIGHT;

    // 1. 照準 (Reticle)
    float cx = scW / 2.0f;
    float cy = scH / 2.0f;
    pGraphics->FillRect(cx - 15.0f, cy - 1.0f, 30.0f, 2.0f, 0x8800FF00); // 横
    pGraphics->FillRect(cx - 1.0f, cy - 15.0f, 2.0f, 30.0f, 0x8800FF00); // 縦
    pGraphics->FillRect(cx - 2.0f, cy - 2.0f, 4.0f, 4.0f, 0xFF00FF00);   // 点

    // 2. ステージ情報 (右上)
    int stage = Game::GetInstance()->GetCurrentStage();
    std::wstring stageStr = L"STAGE: " + std::to_wstring(stage);
    std::wstring enemyStr = L"HOSTILES: " + std::to_wstring(enemyCount);

    // 背景プレート
    pGraphics->FillRect(scW - 300.0f, 20.0f, 280.0f, 80.0f, 0x88000000); // 半透明黒
    pGraphics->DrawRect(scW - 300.0f, 20.0f, 280.0f, 80.0f, 0xFFFFFFFF); // 白枠

    pGraphics->DrawString(stageStr.c_str(), scW - 280.0f, 30.0f, 24.0f, 0xFF00FFFF);
    uint32_t eColor = (enemyCount > 0) ? 0xFFFF4444 : 0xFF00FF00;
    pGraphics->DrawString(enemyStr.c_str(), scW - 280.0f, 60.0f, 28.0f, eColor);

    // 3. プレイヤーHPバー (左下)
    if (playerID != ECSConfig::INVALID_ID) {
        auto& status = registry->GetComponent<StatusComponent>(playerID);
        auto& pComp = registry->GetComponent<PlayerComponent>(playerID);

        float barX = 50.0f;
        float barY = scH - 80.0f;
        float barW = 300.0f;
        float barH = 20.0f;

        // 背景
        pGraphics->FillRect(barX, barY, barW, barH, 0x88444444);

        // 中身
        float ratio = (float)status.hp / (float)status.maxHp;
        uint32_t hpColor = 0xFF0088FF;
        if (ratio < 0.3f) hpColor = 0xFFFF0000;
        else if (ratio < 0.6f) hpColor = 0xFFFFFF00;
        pGraphics->FillRect(barX, barY, barW * ratio, barH, hpColor);

        // 枠
        pGraphics->DrawRect(barX, barY, barW, barH, 0xFFFFFFFF);

        // テキスト
        std::wstring hpText = L"ARMOR: " + std::to_wstring(status.hp);
        pGraphics->DrawString(hpText.c_str(), barX, barY - 30.0f, 20.0f, 0xFFFFFFFF);
    }

    pGraphics->EndDraw2D();
}