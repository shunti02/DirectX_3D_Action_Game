#include "ECS/Systems/PlayerSystem.h" // パスは環境に合わせて調整してください

#include "ECS/World.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/CameraComponent.h"
#include "ECS/Components/StatusComponent.h"
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

        bool isDead = false;
        if (registry->HasComponent<StatusComponent>(id)) {
            if (registry->GetComponent<StatusComponent>(id).hp <= 0) {
                isDead = true;
            }
        }

        // ---------------------------------------------------------
        // 1. 移動入力 (X, Z軸の速度設定)
        // ---------------------------------------------------------
        if (player.isActive && !isDead) {
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

            if (player.isActive && isDead) {
                SwitchCharacter(registry);
            }
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
            // 死んでいないなら復帰、死んでるならそのまま
            if (!isDead) {
                trans.position = { 0.0f, 5.0f, 0.0f };
                player.velocity = { 0.0f, 0.0f, 0.0f };
            }
        }
    }
}

void PlayerSystem::SwitchCharacter(Registry* registry) {
    EntityID nextActiveID = ECSConfig::INVALID_ID;

    // 現在操作中のキャラを探す
    EntityID currentID = ECSConfig::INVALID_ID;
    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (registry->HasComponent<PlayerComponent>(id)) {
            if (registry->GetComponent<PlayerComponent>(id).isActive) {
                currentID = id;
                break;
            }
        }
    }

    // 次の操作候補を探す (生存しているキャラを優先)
    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (id == currentID) continue; // 自分以外
        if (!registry->HasComponent<PlayerComponent>(id)) continue;

        // 生存チェック
        bool isAlive = true;
        if (registry->HasComponent<StatusComponent>(id)) {
            if (registry->GetComponent<StatusComponent>(id).hp <= 0) isAlive = false;
        }

        if (isAlive) {
            nextActiveID = id;
            break;
        }
    }

    // 交代可能なキャラが見つかった場合のみ交代
    if (nextActiveID != ECSConfig::INVALID_ID) {
        if (currentID != ECSConfig::INVALID_ID) {
            registry->GetComponent<PlayerComponent>(currentID).isActive = false;
        }
        registry->GetComponent<PlayerComponent>(nextActiveID).isActive = true;

        // カメラターゲット更新
        for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
            if (registry->HasComponent<CameraComponent>(id)) {
                auto& cam = registry->GetComponent<CameraComponent>(id);
                cam.targetEntityID = nextActiveID;
                break;
            }
        }
    }
}