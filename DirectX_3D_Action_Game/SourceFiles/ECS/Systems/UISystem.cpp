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
#include <cmath>

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

    // 時間経過による点滅用
    static float time = 0.0f;
    time += 0.016f; // 60fps想定の簡易加算

    // Direct2D 描画開始
    pGraphics->BeginDraw2D();

    float scW = (float)Config::SCREEN_WIDTH;
    float scH = (float)Config::SCREEN_HEIGHT;

    // =========================================================
    // 1. ダメージ警告エフェクト (HP低下時)
    // =========================================================
    if (playerID != ECSConfig::INVALID_ID) {
        auto& status = registry->GetComponent<StatusComponent>(playerID);
        float hpRatio = (float)status.hp / (float)status.maxHp;

        if (hpRatio < 0.3f) {
            // HP30%以下で画面端を赤く点滅させる
            float alpha = (sinf(time * 10.0f) + 1.0f) * 0.15f; // 0.0 ~ 0.3
            uint32_t alertColor = (static_cast<uint32_t>(alpha * 255.0f) << 24) | 0x00FF0000;

            // 画面全体を覆う赤矩形 (中央を抜くのはD2D標準では難しいので全体を薄く塗る)
            pGraphics->DrawRect(0, 0, scW, scH, alertColor);

            // "WARNING" 文字
            if (fmodf(time, 1.0f) < 0.5f) {
                pGraphics->DrawString(L"WARNING: HULL CRITICAL", scW / 2.0f - 150.0f, scH / 2.0f - 100.0f, 32.0f, 0xFFFF0000);
            }
        }
    }

    // =========================================================
    // 1. レティクル (画面中央の照準)
    // =========================================================
    float cx = scW / 2.0f;
    float cy = scH / 2.0f;
    uint32_t sightColor = 0xAA00FF00; // 半透明緑

    // 十字を描く (DrawRect = 塗りつぶし)
    pGraphics->DrawRect(cx - 15.0f, cy - 1.0f, 30.0f, 2.0f, sightColor); // 横棒
    pGraphics->DrawRect(cx - 1.0f, cy - 15.0f, 2.0f, 30.0f, sightColor); // 縦棒

    // 中心点 (赤)
    pGraphics->DrawRect(cx - 2.0f, cy - 2.0f, 4.0f, 4.0f, 0xFFFF0000);

    // 外側の括弧 [  ]
    float bracketW = 40.0f;
    float bracketH = 30.0f;
    float gap = 10.0f + sinf(time * 2.0f) * 2.0f; // 呼吸するように動く

    // 左括弧
    pGraphics->DrawRect(cx - bracketW - gap, cy - bracketH, 2.0f, bracketH * 2, sightColor); // 縦
    pGraphics->DrawRect(cx - bracketW - gap, cy - bracketH, 10.0f, 2.0f, sightColor); // 上横
    pGraphics->DrawRect(cx - bracketW - gap, cy + bracketH, 10.0f, 2.0f, sightColor); // 下横

    // 右括弧
    pGraphics->DrawRect(cx + bracketW + gap, cy - bracketH, 2.0f, bracketH * 2, sightColor); // 縦
    pGraphics->DrawRect(cx + bracketW + gap - 10.0f, cy - bracketH, 10.0f, 2.0f, sightColor); // 上横
    pGraphics->DrawRect(cx + bracketW + gap - 10.0f, cy + bracketH, 10.0f, 2.0f, sightColor); // 下横
    // =========================================================
    // 2. ステージ情報 (右上)
    // =========================================================
    int stage = Game::GetInstance()->GetCurrentStage();
    std::wstring stageStr = L"STAGE: " + std::to_wstring(stage);
    std::wstring enemyStr = L"HOSTILES: " + std::to_wstring(enemyCount);

    // 右上の情報パネル
    float infoW = 300.0f;
    float infoX = scW - infoW - 20.0f;

    // パネル背景 (斜めカット風に見せるため、矩形を少しずらして重ねる技法)
    pGraphics->DrawRect(infoX, 20.0f, infoW, 80.0f, 0xAA001133); // 紺色ベース
    pGraphics->DrawRect(infoX, 20.0f, infoW, 2.0f, 0xFF00FFFF);  // 上ライン
    pGraphics->DrawRect(infoX, 98.0f, infoW, 2.0f, 0xFF00FFFF);  // 下ライン

    pGraphics->DrawString(stageStr.c_str(), infoX + 20.0f, 30.0f, 24.0f, 0xFF00FFFF);
    uint32_t eColor = (enemyCount > 0) ? 0xFFFF4444 : 0xFF00FF00;
    pGraphics->DrawString(enemyStr.c_str(), infoX + 20.0f, 60.0f, 28.0f, eColor);

    // =========================================================
    // 3. プレイヤーHPバー (左下)
    // =========================================================
    if (playerID != ECSConfig::INVALID_ID) {
        auto& status = registry->GetComponent<StatusComponent>(playerID);
        // auto& pComp = registry->GetComponent<PlayerComponent>(playerID);

        float barX = 50.0f;
        float barY = scH - 80.0f;
        float barW = 300.0f;
        float barH = 20.0f;

        // ベースパネル
        pGraphics->DrawRect(barX - 20, barY - 40, barW + 40, 70, 0x88000000);
        // 装飾ライン (L字)
        pGraphics->DrawRect(barX - 20, barY - 40, 2, 70, 0xFF00FFFF); // 左縦
        pGraphics->DrawRect(barX - 20, barY + 30, 100, 2, 0xFF00FFFF); // 下横

        // --- HPゲージ ---
        // 背景 (グリッド風に切れ目を入れる)
        for (int i = 0; i < 10; ++i) {
            pGraphics->DrawRect(barX + (i * (barW / 10.0f)), barY, (barW / 10.0f) - 2.0f, barH, 0xFF333333);
        }

        // 現在HP
        float ratio = (float)status.hp / (float)status.maxHp;
        if (ratio < 0) ratio = 0;

        uint32_t hpColor = 0xFF00FF00;
        if (ratio < 0.3f) hpColor = 0xFFFF0000;
        else if (ratio < 0.5f) hpColor = 0xFFFFFF00;

        // ゲージ本体 (連続したバーではなく、ブロック状に塗る)
        int blockCount = (int)(ratio * 10.0f);
        if (blockCount == 0 && ratio > 0) blockCount = 1;

        for (int i = 0; i < blockCount; ++i) {
            pGraphics->DrawRect(barX + (i * (barW / 10.0f)), barY, (barW / 10.0f) - 2.0f, barH, hpColor);
        }

        // テキスト
        std::wstring hpText = L"SHIELD INTEGRITY: " + std::to_wstring((int)(ratio * 100)) + L"%";
        pGraphics->DrawString(hpText.c_str(), barX, barY - 30.0f, 20.0f, hpColor);
    }

    pGraphics->EndDraw2D();
}