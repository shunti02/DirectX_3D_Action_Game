#include "Scene/GameScene.h"
#include "App/Game.h"
#include "Game/EntityFactory.h"
#include "Scene/ResultScene.h"

#include "ECS/Components/StatusComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/EnemyComponent.h"
#include "ECS/Components/CameraComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Systems/ParticleSystem.h"
#include "ECS/Components/MovingComponent.h"

#include "ECS/Systems/RenderSystem.h"
#include "ECS/Systems/CameraSystem.h"
#include "ECS/Systems/PlayerSystem.h"
#include "ECS/Systems/PlayerAnimationSystem.h"
#include "ECS/Systems/EnemySystem.h"
#include "ECS/Systems/EnemyAnimationSystem.h"
#include "ECS/Systems/PhysicsSystem.h"
#include "ECS/Systems/ActionSystem.h"
#include "ECS/Systems/UISystem.h"
#include "ECS/Systems/MovingSystem.h"

#include <iostream>
#include <cstdlib>

void GameScene::Initialize() {
    while (ShowCursor(FALSE) >= 0);

    pWorld->AddSystem<PlayerSystem>()->Init(pWorld.get());
    pWorld->AddSystem<EnemySystem>()->Init(pWorld.get());
    pWorld->AddSystem<ActionSystem>()->Init(pWorld.get());
    pWorld->AddSystem<PhysicsSystem>()->Init(pWorld.get());
    pWorld->AddSystem<ParticleSystem>()->Init(pWorld.get());
    pWorld->AddSystem<MovingSystem>()->Init(pWorld.get());
    m_pEnemyAnimSystem = pWorld->AddSystem<EnemyAnimationSystem>();
    m_pEnemyAnimSystem->Init(pWorld.get());

    m_pAnimSystem = pWorld->AddSystem<PlayerAnimationSystem>();
    m_pAnimSystem->Init(pWorld.get());

    m_pCameraSystem = pWorld->AddSystem<CameraSystem>();
    m_pCameraSystem->Init(pWorld.get());

    m_pRenderSystem = pWorld->AddSystem<RenderSystem>();
    m_pRenderSystem->Init(pWorld.get());

    pUISystem = pWorld->AddSystem<UISystem>();
    pUISystem->Init(pWorld.get());

    m_isSceneChanging = false;
    m_startTimer = 0.0f;
    m_isGameOverSequence = false;
    m_gameOverTimer = 0.0f;

    int currentStage = Game::GetInstance()->GetCurrentStage();
    AppLog::AddLog("--- STAGE %d START ---", currentStage);

    DirectX::XMFLOAT3 startPos = { 0.0f, 0.5f, -15.0f };

    //プレイヤー生成
    CustomizeData currentConfig = Game::GetInstance()->GetCustomizeData();
    EntitySpawnParams pParams;
    pParams.type = "Player";
    pParams.position = startPos;
    pParams.scale = { 1.0f, 1.0f, 1.0f };
    pParams.isGrounded = false;

    EntityID playerID = EntityFactory::AssembleCustomPlayer(pWorld.get(), pParams, currentConfig);
    auto& pComp = pWorld->GetComponent<PlayerComponent>(playerID);

    StatusComponent status;
    status.maxHp = (int)pComp.maxHp;
    status.attackPower = (int)pComp.totalAttack;
    status.hp = status.maxHp;

    pWorld->AddComponent<StatusComponent>(playerID, status);
    pWorld->AddComponent<ColliderComponent>(playerID);
    pWorld->GetComponent<ColliderComponent>(playerID).SetSphere(0.6f);
    pWorld->AddComponent<ActionComponent>(playerID, ActionComponent{ .attackCooldown = 1.0f, .duration = 0.5f });

    //カメラ生成
    EntityID cameraID = EntityFactory::CreateEntity(pWorld.get(), {
        .type = "Camera",
        .position = { 0.0f, 0.0f, 0.0f }
        });
    pWorld->GetComponent<CameraComponent>(cameraID).targetEntityID = playerID;

    EntityFactory::CreateEntity(pWorld.get(), {
        .type = "Ground",
        .position = {0.0f, -1.0f, 0.0f},
        .scale = {80.0f, 1.0f, 80.0f}
        });
    float radius = 38.0f;
    int segments = 36;

    for (int i = 0; i < segments; ++i) {
        float angle = (DirectX::XM_2PI / segments) * i;
        float x = cosf(angle) * radius;
        float z = sinf(angle) * radius;
        float rotY = -angle + DirectX::XM_PIDIV2;

        EntityFactory::CreateEntity(pWorld.get(), {
            .type = "Boundary",
            .position = {x, 2.0f, z},
            .rotation = {0.0f, rotY, 0.0f},
            .scale = {6.8f, 2.0f, 0.1f}
            });
    }
    CustomizeData meleeConfig = { 3, 4, 4, 4, 4, 2, -1, 2 };
    CustomizeData sniperConfig = { 2, 2, 2, 2, 2, 2, -1, 1 };

    if (currentStage == 1) {
        EntityID e1 = EntityFactory::AssembleCustomPlayer(pWorld.get(), { .position = {0.0f, 0.5f, 50.0f} }, meleeConfig, true);
        pWorld->GetComponent<EnemyComponent>(e1).type = EnemyType::Normal;
    }
    else if (currentStage == 2) {
        EntityID e1 = EntityFactory::AssembleCustomPlayer(pWorld.get(), { .position = {-8.0f, 0.5f, 10.0f} }, sniperConfig, true);
        pWorld->GetComponent<EnemyComponent>(e1).type = EnemyType::Ranged;
        EntityFactory::CreateEntity(pWorld.get(), { .type = "EnemyTurret", .position = {8.0f, 0.5f, 15.0f} });
    }
    else if (currentStage == 3) {
        EntityID e1 = EntityFactory::AssembleCustomPlayer(pWorld.get(), { .position = {-10.0f, 0.5f, 10.0f} }, meleeConfig, true);
        pWorld->GetComponent<EnemyComponent>(e1).type = EnemyType::Normal;
        EntityID e2 = EntityFactory::AssembleCustomPlayer(pWorld.get(), { .position = {10.0f, 0.5f, 10.0f} }, sniperConfig, true);
        pWorld->GetComponent<EnemyComponent>(e2).type = EnemyType::Ranged;
        EntityFactory::CreateEntity(pWorld.get(), { .type = "EnemyTurret", .position = {0.0f, 0.5f, 20.0f} });
    }

    //SkyBox
    pSkyBox = std::make_unique<SkyBox>();
    if (!pSkyBox->Initialize(Game::GetInstance()->GetGraphics())) { pSkyBox.reset(); }
    if (auto audio = Game::GetInstance()->GetAudio()) {
        audio->Play("BGM_GAME", true, 0.4f);
    }
}

void GameScene::Update(float dt) {
    m_startTimer += dt;
    float waitTime = 3.0f;

    if (m_startTimer < waitTime) {
        auto registry = pWorld->GetRegistry();
        for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
            if (registry->HasComponent<PlayerComponent>(id)) {
                registry->GetComponent<PlayerComponent>(id).isGrounded = true;
                if (registry->HasComponent<PhysicsComponent>(id)) {
                    registry->GetComponent<PhysicsComponent>(id).velocity = { 0, 0, 0 };
                }
            }
        }
        if (m_pAnimSystem) m_pAnimSystem->Update(dt);
        if (m_pEnemyAnimSystem) m_pEnemyAnimSystem->Update(dt);
        if (m_pCameraSystem) m_pCameraSystem->Update(dt);
        if (m_pRenderSystem) m_pRenderSystem->Update(dt);
    }
    else {
        BaseScene::Update(dt);
        if (m_isGameOverSequence) {
            m_gameOverTimer += dt;

            if (m_gameOverTimer >= 5.0f) {
                ResultScene::isClear = false;
                Game::GetInstance()->GetSceneManager()->ChangeScene<ResultScene>();
            }
        }
        else {
            CheckGameCondition();
        }
    }
}

void GameScene::Draw() {
    BaseScene::Draw();

    auto registry = pWorld->GetRegistry();
    DirectX::XMMATRIX view = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX proj = DirectX::XMMatrixIdentity();

    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (registry->HasComponent<CameraComponent>(id)) {
            auto& cam = registry->GetComponent<CameraComponent>(id);
            view = cam.view;
            proj = cam.projection;
            break;
        }
    }

    if (pSkyBox) pSkyBox->Draw(Game::GetInstance()->GetGraphics(), view, proj);

    Game::GetInstance()->GetGraphics()->GetContext()->OMSetDepthStencilState(nullptr, 0);
    if (pUISystem) pUISystem->Draw(Game::GetInstance()->GetGraphics());

    Graphics* g = Game::GetInstance()->GetGraphics();
    if (g) {
        g->BeginDraw2D();
        if (m_startTimer < 2.0f) {
            g->DrawString(L"READY...", 500.0f, 300.0f, 80.0f, 0xFFFFFFFF);
        }
        else if (m_startTimer < 3.0f) {
            float alpha = 1.0f - (m_startTimer - 2.0f);
            uint32_t a = static_cast<uint32_t>(alpha * 255.0f);
            uint32_t col = (a << 24) | 0xFF0000;
            g->DrawString(L"GO !!", 550.0f, 300.0f, 100.0f + (m_startTimer - 2.0f) * 50.0f, col);
        }
        if (m_isGameOverSequence) {
            g->DrawString(L"MISSION FAILED", 450.0f, 300.0f, 80.0f, 0xFF0000FF);
        }
        g->EndDraw2D();
    }
}

void GameScene::CheckGameCondition() {
    if (m_isSceneChanging) return;
    auto registry = pWorld->GetRegistry();

    int alivePlayers = 0;
    int remainingEnemies = 0;

    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (registry->HasComponent<StatusComponent>(id)) {
            auto& status = registry->GetComponent<StatusComponent>(id);
            if (status.hp <= 0) continue;

            if (registry->HasComponent<PlayerComponent>(id)) alivePlayers++;
            else if (registry->HasComponent<EnemyComponent>(id)) remainingEnemies++;
        }
    }
    if (alivePlayers == 0) {
        m_isSceneChanging = true;
        m_isGameOverSequence = true;
        return;
    }
    if (remainingEnemies == 0) {
        m_isSceneChanging = true;
        Game::GetInstance()->UnlockNextStage();
        Game::GetInstance()->SaveGame();
        ResultScene::isClear = true;
        Game::GetInstance()->GetSceneManager()->ChangeScene<ResultScene>();
    }
}

void GameScene::Shutdown() {
    while (ShowCursor(TRUE) < 0);
    if (auto audio = Game::GetInstance()->GetAudio()) {
        audio->Stop("BGM_GAME");
    }
}