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

void UISystem::Init(World* world) {
    pWorld = world;
}
std::wstring ToWString(const std::string& str) {
    size_t newsize = str.length() + 1;
    std::wstring wc(newsize, L'\0');
    size_t convertedChars = 0;
    mbstowcs_s(&convertedChars, &wc[0], newsize, str.c_str(), _TRUNCATE);
    return wc;
}

void UISystem::Update(float dt) {
    auto registry = pWorld->GetRegistry();
}

void UISystem::Draw(Graphics* pGraphics) {
    if (!pGraphics) return;

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

    static float time = 0.0f;
    time += 0.016f;

    pGraphics->BeginDraw2D();

    float scW = (float)Config::SCREEN_WIDTH;
    float scH = (float)Config::SCREEN_HEIGHT;

    if (playerID != ECSConfig::INVALID_ID) {
        auto& status = registry->GetComponent<StatusComponent>(playerID);
        float hpRatio = (float)status.hp / (float)status.maxHp;

        if (hpRatio < 0.3f) {
            float alpha = (sinf(time * 10.0f) + 1.0f) * 0.15f;
            uint32_t alertColor = (static_cast<uint32_t>(alpha * 255.0f) << 24) | 0x00FF0000;

            pGraphics->DrawRect(0, 0, scW, scH, alertColor);

            if (fmodf(time, 1.0f) < 0.5f) {
                pGraphics->DrawString(L"WARNING: HULL CRITICAL", scW / 2.0f - 150.0f, scH / 2.0f - 100.0f, 32.0f, 0xFFFF0000);
            }
        }
    }

    float cx = scW / 2.0f;
    float cy = scH / 2.0f;
    uint32_t sightColor = 0xAA00FF00;

    pGraphics->DrawRect(cx - 15.0f, cy - 1.0f, 30.0f, 2.0f, sightColor);
    pGraphics->DrawRect(cx - 1.0f, cy - 15.0f, 2.0f, 30.0f, sightColor);

    pGraphics->DrawRect(cx - 2.0f, cy - 2.0f, 4.0f, 4.0f, 0xFFFF0000);

    float bracketW = 40.0f;
    float bracketH = 30.0f;
    float gap = 10.0f + sinf(time * 2.0f) * 2.0f; 

    pGraphics->DrawRect(cx - bracketW - gap, cy - bracketH, 2.0f, bracketH * 2, sightColor);
    pGraphics->DrawRect(cx - bracketW - gap, cy - bracketH, 10.0f, 2.0f, sightColor);
    pGraphics->DrawRect(cx - bracketW - gap, cy + bracketH, 10.0f, 2.0f, sightColor);

    pGraphics->DrawRect(cx + bracketW + gap, cy - bracketH, 2.0f, bracketH * 2, sightColor);
    pGraphics->DrawRect(cx + bracketW + gap - 10.0f, cy - bracketH, 10.0f, 2.0f, sightColor);
    pGraphics->DrawRect(cx + bracketW + gap - 10.0f, cy + bracketH, 10.0f, 2.0f, sightColor);
    int stage = Game::GetInstance()->GetCurrentStage();
    std::wstring stageStr = L"STAGE: " + std::to_wstring(stage);
    std::wstring enemyStr = L"HOSTILES: " + std::to_wstring(enemyCount);

    float infoW = 300.0f;
    float infoX = scW - infoW - 20.0f;

    pGraphics->DrawRect(infoX, 20.0f, infoW, 80.0f, 0xAA001133);
    pGraphics->DrawRect(infoX, 20.0f, infoW, 2.0f, 0xFF00FFFF);
    pGraphics->DrawRect(infoX, 98.0f, infoW, 2.0f, 0xFF00FFFF);

    pGraphics->DrawString(stageStr.c_str(), infoX + 20.0f, 30.0f, 24.0f, 0xFF00FFFF);
    uint32_t eColor = (enemyCount > 0) ? 0xFFFF4444 : 0xFF00FF00;
    pGraphics->DrawString(enemyStr.c_str(), infoX + 20.0f, 60.0f, 28.0f, eColor);

    if (playerID != ECSConfig::INVALID_ID) {
        auto& status = registry->GetComponent<StatusComponent>(playerID);

        float barX = 50.0f;
        float barY = scH - 80.0f;
        float barW = 300.0f;
        float barH = 20.0f;

        pGraphics->DrawRect(barX - 20, barY - 40, barW + 40, 70, 0x88000000);
        pGraphics->DrawRect(barX - 20, barY - 40, 2, 70, 0xFF00FFFF);
        pGraphics->DrawRect(barX - 20, barY + 30, 100, 2, 0xFF00FFFF);

        for (int i = 0; i < 10; ++i) {
            pGraphics->DrawRect(barX + (i * (barW / 10.0f)), barY, (barW / 10.0f) - 2.0f, barH, 0xFF333333);
        }

        float ratio = (float)status.hp / (float)status.maxHp;
        if (ratio < 0) ratio = 0;

        uint32_t hpColor = 0xFF00FF00;
        if (ratio < 0.3f) hpColor = 0xFFFF0000;
        else if (ratio < 0.5f) hpColor = 0xFFFFFF00;

        int blockCount = (int)(ratio * 10.0f);
        if (blockCount == 0 && ratio > 0) blockCount = 1;

        for (int i = 0; i < blockCount; ++i) {
            pGraphics->DrawRect(barX + (i * (barW / 10.0f)), barY, (barW / 10.0f) - 2.0f, barH, hpColor);
        }

        std::wstring hpText = L"SHIELD INTEGRITY: " + std::to_wstring((int)(ratio * 100)) + L"%";
        pGraphics->DrawString(hpText.c_str(), barX, barY - 30.0f, 20.0f, hpColor);
    }

    pGraphics->EndDraw2D();
}