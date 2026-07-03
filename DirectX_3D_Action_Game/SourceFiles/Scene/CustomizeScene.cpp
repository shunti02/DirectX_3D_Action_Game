/*===================================================================
// ファイル: CustomizeScene.cpp
// 概要: カスタマイズ画面（左右腕・脚独立 6カテゴリー対応版）
=====================================================================*/
#include "Scene/CustomizeScene.h"
#include "Scene/StageSelectScene.h"
#include "Engine/Input.h"
#include "App/Main.h"
#include "Game/EntityFactory.h" 
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/PlayerPartComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Systems/PlayerAnimationSystem.h"
#include "ECS/Systems/RenderSystem.h"

CustomizeScene::CustomizeScene(SceneManager* manager) : BaseScene(manager) {}

void CustomizeScene::Initialize() {
    m_currentConfig = Game::GetInstance()->GetCustomizeData();

    pSkyBox = std::make_unique<SkyBox>();
    if (!pSkyBox->Initialize(Game::GetInstance()->GetGraphics())) { pSkyBox.reset(); }

    m_pAnimSystem = pWorld->AddSystem<PlayerAnimationSystem>();
    m_pAnimSystem->Init(pWorld.get());

    auto renderSys = pWorld->AddSystem<RenderSystem>();
    renderSys->Init(pWorld.get());

    UpdatePreviewMesh();

    EntityFactory::CreateEntity(pWorld.get(), { .type = "Camera", .position = { 0.0f, 0.5f, -8.0f } });
    AppLog::AddLog("--- Customize Scene Initialized ---");

    if (auto audio = Game::GetInstance()->GetAudio()) {
        audio->Play("BGM_CUSTOMIZE", true, 0.4f);
    }
}

void CustomizeScene::Update(float dt) {
    m_blinkTimer += dt;
    Input* input = Game::GetInstance()->GetInput();
    bool changed = false;

    // 1. 部位（カテゴリー）の選択 (上下キー)
    if (input->IsKeyDown('W') || input->IsKeyDown(VK_UP)) {
        m_selectedCategory = (m_selectedCategory - 1 + 8) % 8;
        if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_SWITCH");
    }
    if (input->IsKeyDown('S') || input->IsKeyDown(VK_DOWN)) {
        m_selectedCategory = (m_selectedCategory + 1) % 8;
        if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_SWITCH");
    }

    // 2. パーツモデルの変更 (左右キーで 0 ~ 4 に切り替え)
    int* pTargetID = nullptr;
    int maxItems = 5;
    if (m_selectedCategory == 0) pTargetID = &m_currentConfig.headID;
    else if (m_selectedCategory == 1) pTargetID = &m_currentConfig.bodyID;
    else if (m_selectedCategory == 2) pTargetID = &m_currentConfig.waistID;
    else if (m_selectedCategory == 3) pTargetID = &m_currentConfig.armLeftID;
    else if (m_selectedCategory == 4) pTargetID = &m_currentConfig.armRightID;
    else if (m_selectedCategory == 5) pTargetID = &m_currentConfig.legID;
    else if (m_selectedCategory == 6) {
        pTargetID = &m_currentConfig.weaponRightID;
        maxItems = 3; // ★右武器は3種類
    }
    else if (m_selectedCategory == 7) {
        pTargetID = &m_currentConfig.weaponLeftID;
        maxItems = 3; // ★左武器は3種類
    }
    if (pTargetID) {
        // ★修正: ハードコードされた 5 ではなく、maxItems 変数を使用して計算する
        if (input->IsKeyDown('A') || input->IsKeyDown(VK_LEFT)) {
            *pTargetID = (*pTargetID - 1 + maxItems) % maxItems;
            changed = true;
        }
        if (input->IsKeyDown('D') || input->IsKeyDown(VK_RIGHT)) {
            *pTargetID = (*pTargetID + 1) % maxItems;
            changed = true;
        }
    }

    if (changed) {
        if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_SWITCH");
        UpdatePreviewMesh();
    }

    if (previewModelID != -1) {
        auto& trans = pWorld->GetComponent<TransformComponent>((EntityID)previewModelID);
        trans.rotation.y += 0.8f * dt;
    }

    if (m_pAnimSystem) m_pAnimSystem->Update(dt);
    if (pWorld) pWorld->Update(dt);

    auto registry = pWorld->GetRegistry();
    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (registry->HasComponent<CameraComponent>(id)) {
            auto& cam = registry->GetComponent<CameraComponent>(id);
            XMVECTOR eye = DirectX::XMVectorSet(0.0f, 1.2f, -6.0f, 0.0f);
            XMVECTOR focus = DirectX::XMVectorSet(0.0f, 0.8f, 0.0f, 0.0f);
            XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            cam.view = DirectX::XMMatrixLookAtLH(eye, focus, up);
            cam.projection = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(45.0f), Config::SCREEN_WIDTH / (float)Config::SCREEN_HEIGHT, 0.1f, 1000.0f);
            break;
        }
    }

    if (input->IsKeyDown(VK_RETURN) || input->IsKeyDown(VK_SPACE)) {
        Game::GetInstance()->SetCustomizeData(m_currentConfig);
        Game::GetInstance()->SetCurrentStage(1);
        Game::GetInstance()->GetSceneManager()->ChangeScene<StageSelectScene>();
        if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_JUMP");
    }
}

void CustomizeScene::UpdatePreviewMesh() {
    auto registry = pWorld->GetRegistry();
    std::vector<EntityID> partsToDelete;

    for (EntityID id = 0; id < 100000; ++id) {
        if (registry->HasComponent<PlayerPartComponent>(id)) {
            partsToDelete.push_back(id);
        }
    }
    for (auto id : partsToDelete) pWorld->DestroyEntity(id);

    if (previewModelID != -1) {
        pWorld->DestroyEntity((EntityID)previewModelID);
        previewModelID = -1;
    }

    EntitySpawnParams params;
    params.position = { 0.0f, 0.0f, 0.0f };
    params.scale = { 1.0f, 1.0f, 1.0f };
    params.isGrounded = true;

    previewModelID = (int)EntityFactory::AssembleCustomPlayer(pWorld.get(), params, m_currentConfig);
}

void CustomizeScene::Draw() {
    Graphics* pGraphics = Game::GetInstance()->GetGraphics();
    if (pSkyBox) {
        XMMATRIX view = DirectX::XMMatrixLookAtLH(DirectX::XMVectorSet(0.0f, 1.2f, -6.0f, 0.0f), DirectX::XMVectorSet(0.0f, 0.8f, 0.0f, 0.0f), DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
        XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(45.0f), Config::SCREEN_WIDTH / (float)Config::SCREEN_HEIGHT, 0.1f, 1000.0f);
        pSkyBox->Draw(pGraphics, view, proj);
    }

    BaseScene::Draw();

    if (pGraphics) {
        pGraphics->BeginDraw2D();
        float scW = (float)Config::SCREEN_WIDTH;

        pGraphics->DrawString(L"MACHINE CUSTOMIZE", 50.0f, 40.0f, 36.0f, 0xFFFFFFFF);
        pGraphics->DrawRect(0, 90.0f, scW, 2.0f, 0xFF00FFFF);

        const wchar_t* categories[] = {
            L"HEAD(頭部)",
            L"BODY(胴体)",
            L"WAIST(腰部)",
            L"L-ARM(左腕)",
            L"R-ARM(右腕)",
            L"LEG(脚部)",
            L"R-WEAPON(右武器)", // ←追加
            L"L-WEAPON(左武器)"
        };

        int currentIDs[] = {
            m_currentConfig.headID,
            m_currentConfig.bodyID,
            m_currentConfig.waistID,
            m_currentConfig.armLeftID,
            m_currentConfig.armRightID,
            m_currentConfig.legID,
            m_currentConfig.weaponRightID, // ←追加
            m_currentConfig.weaponLeftID
        };

        const wchar_t* partNames[] = {
            L" Type-A [Custom]",
            L" Type-B [Heavy]",
            L" Type-C [Light]",
            L" Type-D [Assassin]",
            L" Type-E [Fortress]"
        };

        const wchar_t* weaponNames[] = {
            L" Assault Rifle(実弾)",
            L" Laser Rifle(光学)",
            L" Beam Saber(近接)"
        };

        float slotY = 120.0f;
        for (int i = 0; i < 8; ++i) {
            uint32_t col = (m_selectedCategory == i) ? 0xFF00FFFF : 0xFF888888;
            pGraphics->DrawString(categories[i], 50.0f, slotY, 24.0f, col);

            // 描画する名前をカテゴリーで分岐
            if (i == 6 || i == 7) {
                // 武器カテゴリーの場合
                pGraphics->DrawString(weaponNames[currentIDs[i]], 270.0f, slotY, 24.0f, (m_selectedCategory == i) ? 0xFFFFFFFF : 0xFFCCCCCC);
            }
            else {
                // 装甲パーツカテゴリーの場合
                pGraphics->DrawString(partNames[currentIDs[i]], 270.0f, slotY, 24.0f, (m_selectedCategory == i) ? 0xFFFFFFFF : 0xFFCCCCCC);
            }
            slotY += 45.0f; // ★間隔を 50.0f から 45.0f に縮小
        }

        if (previewModelID != -1 && pWorld->HasComponent<PlayerComponent>((EntityID)previewModelID)) {
            auto& pComp = pWorld->GetComponent<PlayerComponent>((EntityID)previewModelID);

            float statX = scW - 350.0f;
            float statY = 150.0f;

            // ★修正: 新しいパラメータを表示するため枠を縦に伸ばす (300 -> 380)
            pGraphics->FillRect(statX - 20, statY - 20, 330.0f, 380.0f, 0xAA001133);
            pGraphics->DrawRectOutline(statX - 20, statY - 20, 330.0f, 380.0f, 1.0f, 0xFF00FFFF);

            pGraphics->DrawString(L"TOTAL STATUS", statX, statY, 24.0f, 0xFFFFD700);

            auto DrawStatRow = [&](const wchar_t* label, float val, float max, float y, uint32_t col) {
                pGraphics->DrawString(label, statX, y, 20.0f, 0xFFFFFFFF);
                // ゲージが枠を突き抜けないように安全対策
                float ratio = val / max;
                if (ratio > 1.0f) ratio = 1.0f;
                if (ratio < 0.0f) ratio = 0.0f;
                pGraphics->FillRect(statX + 80.0f, y + 4, ratio * 180.0f, 12.0f, col);
                pGraphics->DrawRectOutline(statX + 80.0f, y + 4, 180.0f, 12.0f, 1.0f, 0xFF444444);
                };

            DrawStatRow(L"HP", pComp.maxHp, 250.0f, statY + 50.0f, 0xFF00FF00);
            DrawStatRow(L"ATK", pComp.totalAttack, 50.0f, statY + 90.0f, 0xFFFF0000);
            DrawStatRow(L"DEF", pComp.totalDefense, 50.0f, statY + 130.0f, 0xFF00FFFF);
            DrawStatRow(L"WGT", pComp.totalWeight, 150.0f, statY + 170.0f, 0xFFFFAA00);
            DrawStatRow(L"SPD", pComp.moveSpeed, 10.0f, statY + 210.0f, 0xFF00FF7F);

            // ★新規追加パラメータの描画
            DrawStatRow(L"ACCEL", pComp.acceleration, 30.0f, statY + 250.0f, 0xFFFF00FF);  // 加速度（動き出し）
            DrawStatRow(L"BRAKE", pComp.deceleration, 30.0f, statY + 290.0f, 0xFFDDDDDD);  // 減速度（ブレーキ）
            DrawStatRow(L"TURN", pComp.turnSpeed, 20.0f, statY + 330.0f, 0xFFFFFF00);      // 旋回速度
        }

        float alpha = (sinf(m_blinkTimer * 5.0f) + 1.0f) * 0.5f;
        uint32_t pressColor = (static_cast<uint32_t>(alpha * 255.0f) << 24) | 0x00FFFFFF;
        pGraphics->DrawString(L"PRESS ENTER TO ASSEMBLE & LAUNCH", scW / 2.0f - 240.0f, 650.0f, 24.0f, pressColor);

        pGraphics->EndDraw2D();
    }
}

void CustomizeScene::Shutdown() {
    pSkyBox.reset();
    if (auto audio = Game::GetInstance()->GetAudio()) {
        audio->Stop("BGM_CUSTOMIZE");
    }
}