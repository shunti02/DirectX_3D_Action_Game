#include "ECS/Systems/PlayerSystem.h" // パスは環境に合わせて調整してください

#include "ECS/World.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/CameraComponent.h"
#include "App/Game.h"

void PlayerSystem::Update(float dt) {
    auto registry = pWorld->GetRegistry();
    Input* input = Game::GetInstance()->GetInput();

    // --- キャラ切り替え (Tabキー) ---
    if (input->IsKeyDown(VK_TAB)) {
        if (!tabKeyPressed) {
            tabKeyPressed = true;
            SwitchCharacter(registry);
        }
    }
    else {
        tabKeyPressed = false;
    }

    // --- プレイヤー制御 ---
    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (!registry->HasComponent<PlayerComponent>(id)) continue;

        auto& trans = registry->GetComponent<TransformComponent>(id);
        auto& player = registry->GetComponent<PlayerComponent>(id);

        // ---------------------------------------------------------
        // 1. 移動入力 (X, Z軸の速度設定)
        // ---------------------------------------------------------
        if (player.isActive) {
            player.velocity.x = 0.0f;
            player.velocity.z = 0.0f;

            if (input->IsKey('W')) player.velocity.z = player.moveSpeed;
            if (input->IsKey('S')) player.velocity.z = -player.moveSpeed;
            if (input->IsKey('A')) player.velocity.x = -player.moveSpeed;
            if (input->IsKey('D')) player.velocity.x = player.moveSpeed;

            // ---------------------------------------------------------
            // 2. ジャンプ (接地している時だけ)
            // ---------------------------------------------------------
            if (input->IsKeyDown(VK_SPACE) && player.isGrounded) {
                player.velocity.y = player.jumpPower; // 上方向へドーン！
                player.isGrounded = false; // 空中に飛び出したのでfalse
            }
        }
        else {
            // 操作してないキャラは停止
            player.velocity.x = 0.0f;
            player.velocity.z = 0.0f;
        }

        // ---------------------------------------------------------
        // 3. 重力加算 (Y軸の速度変化)
        // ---------------------------------------------------------
        // 常に下へ引っ張る
        player.velocity.y -= player.gravity * dt;

        // ---------------------------------------------------------
        // 4. 速度を位置に反映 (積分)
        // ---------------------------------------------------------
        trans.position.x += player.velocity.x * dt;
        trans.position.y += player.velocity.y * dt;
        trans.position.z += player.velocity.z * dt;

        // 毎フレーム「今は空中にいる」と仮定してリセットする
        // (PhysicsSystemでの接地判定実装待ち)
        player.isGrounded = false;

        // 安全策: 地面より下（奈落）に落ちたらリセット
        if (trans.position.y < -10.0f) {
            trans.position = { 0.0f, 5.0f, 0.0f }; // 上空へ戻す
            player.velocity = { 0.0f, 0.0f, 0.0f };
        }
    }
}

void PlayerSystem::SwitchCharacter(Registry* registry) {
    EntityID nextActiveID = ECSConfig::INVALID_ID;

    // 全プレイヤーの isActive を反転
    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (registry->HasComponent<PlayerComponent>(id)) {
            auto& player = registry->GetComponent<PlayerComponent>(id);
            player.isActive = !player.isActive;

            if (player.isActive) {
                nextActiveID = id;
                // DebugLog関数が未定義の場合は適宜修正してください
                // DebugLog("Switched to Player ID: %d", id);
            }
        }
    }

    // カメラターゲット更新
    if (nextActiveID != ECSConfig::INVALID_ID) {
        for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
            if (registry->HasComponent<CameraComponent>(id)) {
                auto& cam = registry->GetComponent<CameraComponent>(id);
                cam.targetEntityID = nextActiveID;
                break;
            }
        }
    }
}