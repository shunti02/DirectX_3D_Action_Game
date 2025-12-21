/*===================================================================
// ファイル: PlayerSystem.cpp
// 概要: プレイヤーの入力・移動・交代制御
=====================================================================*/
#include "ECS/Systems/PlayerSystem.h"
#include "ECS/World.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/CameraComponent.h"
#include "ECS/Components/StatusComponent.h"
#include "App/Game.h"
#include "App/Main.h"
#include <cmath>
#include <algorithm>
#include <cstdio> // printf用

using namespace DirectX;

// 線形補間
static float Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

// 待機場所 (画面外)
static const float STANDBY_Y = -50.0f;

// ---------------------------------------------------------
// Update
// ---------------------------------------------------------
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

    // カメラ角度取得
    float cameraYaw = 0.0f;
    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (registry->HasComponent<CameraComponent>(id)) {
            cameraYaw = registry->GetComponent<CameraComponent>(id).angleY;
            break;
        }
    }

    // --- プレイヤー制御ループ ---
    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (!registry->HasComponent<PlayerComponent>(id)) continue;

        auto& trans = registry->GetComponent<TransformComponent>(id);
        auto& player = registry->GetComponent<PlayerComponent>(id);

        bool isDead = false;
        if (registry->HasComponent<StatusComponent>(id)) {
            if (registry->GetComponent<StatusComponent>(id).hp <= 0) isDead = true;
        }

        // =========================================================
        // A. アクティブなキャラ (操作中)
        // =========================================================
        if (player.isActive && !isDead) {
            // 接地中は操作可能
            if (player.isGrounded) {
                float inputX = 0.0f;
                float inputZ = 0.0f;
                if (input->IsKey('W')) inputZ += 1.0f;
                if (input->IsKey('S')) inputZ -= 1.0f;
                if (input->IsKey('A')) inputX -= 1.0f;
                if (input->IsKey('D')) inputX += 1.0f;

                if (inputX != 0.0f || inputZ != 0.0f) {
                    float length = std::sqrt(inputX * inputX + inputZ * inputZ);
                    if (length > 0.0f) { inputX /= length; inputZ /= length; }

                    float sinY = sinf(cameraYaw);
                    float cosY = cosf(cameraYaw);
                    float moveX = inputX * cosY + inputZ * sinY;
                    float moveZ = inputZ * cosY - inputX * sinY;

                    player.velocity.x = moveX * player.moveSpeed;
                    player.velocity.z = moveZ * player.moveSpeed;
                    trans.rotation.y = atan2f(player.velocity.x, player.velocity.z);
                }
                else {
                    player.velocity.x = 0.0f;
                    player.velocity.z = 0.0f;
                }

                if (input->IsKeyDown(VK_SPACE)) {
                    player.velocity.y = player.jumpPower;
                    player.isGrounded = false;
                }
            }
            else {
                // 空中制御 (少しだけ)
                float drag = 2.0f * dt;
                player.velocity.x = Lerp(player.velocity.x, 0.0f, drag);
                player.velocity.z = Lerp(player.velocity.z, 0.0f, drag);

                // 空中移動 (Air Control)
                float speedSq = player.velocity.x * player.velocity.x + player.velocity.z * player.velocity.z;
                if (speedSq < (player.moveSpeed * player.moveSpeed) + 5.0f) {
                    float inputX = 0.0f; float inputZ = 0.0f;
                    if (input->IsKey('W')) inputZ += 1.0f;
                    if (input->IsKey('S')) inputZ -= 1.0f;
                    if (input->IsKey('A')) inputX -= 1.0f;
                    if (input->IsKey('D')) inputX += 1.0f;

                    if (inputX != 0.0f || inputZ != 0.0f) {
                        float length = std::sqrt(inputX * inputX + inputZ * inputZ);
                        if (length > 0.0f) { inputX /= length; inputZ /= length; }
                        float sinY = sinf(cameraYaw); float cosY = cosf(cameraYaw);
                        float moveX = inputX * cosY + inputZ * sinY;
                        float moveZ = inputZ * cosY - inputX * sinY;

                        float airAccel = 10.0f * dt;
                        player.velocity.x += moveX * airAccel;
                        player.velocity.z += moveZ * airAccel;
                        trans.rotation.y = atan2f(moveX, moveZ);
                    }
                }
            }
        }
        // =========================================================
        // B. 非アクティブなキャラ (交代中 or 待機中)
        // =========================================================
        else {
            // 待機場所にいるなら固定
            if (trans.position.y <= STANDBY_Y + 5.0f) {
                player.velocity = { 0.0f, 0.0f, 0.0f };
                trans.position.y = STANDBY_Y;
            }
            // 地上付近なら、そのまま物理挙動で飛ばす (着地したら回収)
            else {
                if (player.isGrounded) {
                    trans.position.y = STANDBY_Y;
                    player.velocity = { 0.0f, 0.0f, 0.0f };
                }
            }

            // 死んだら強制交代
            if (player.isActive && isDead) {
                SwitchCharacter(registry);
            }
        }

        // 共通: 重力と反映
        if (trans.position.y > STANDBY_Y + 1.0f) {
            player.velocity.y -= player.gravity * dt;
        }

        trans.position.x += player.velocity.x * dt;
        trans.position.y += player.velocity.y * dt;
        trans.position.z += player.velocity.z * dt;

        player.isGrounded = false; // PhysicsSystemで更新される

        // 奈落リセット
        if (trans.position.y < STANDBY_Y - 20.0f) {
            trans.position.y = STANDBY_Y;
            player.velocity = { 0.0f, 0.0f, 0.0f };
        }
    }
}

// ---------------------------------------------------------
// SwitchCharacter (交代演出ロジック)
// ---------------------------------------------------------
void PlayerSystem::SwitchCharacter(Registry* registry) {
    EntityID nextID = ECSConfig::INVALID_ID;
    EntityID currentID = ECSConfig::INVALID_ID;

    // 現在のキャラ取得
    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (registry->HasComponent<PlayerComponent>(id)) {
            if (registry->GetComponent<PlayerComponent>(id).isActive) {
                currentID = id;
                break;
            }
        }
    }

    // 次のキャラ取得
    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (id == currentID) continue;
        if (!registry->HasComponent<PlayerComponent>(id)) continue;

        bool isAlive = true;
        if (registry->HasComponent<StatusComponent>(id)) {
            if (registry->GetComponent<StatusComponent>(id).hp <= 0) isAlive = false;
        }
        if (isAlive) {
            nextID = id;
            break;
        }
    }

    // 交代実行
    if (nextID != ECSConfig::INVALID_ID && currentID != ECSConfig::INVALID_ID) {
        auto& currentP = registry->GetComponent<PlayerComponent>(currentID);
        auto& currentT = registry->GetComponent<TransformComponent>(currentID);
        auto& nextP = registry->GetComponent<PlayerComponent>(nextID);
        auto& nextT = registry->GetComponent<TransformComponent>(nextID);

        // 1. 退出 (大ジャンプ)
        currentP.isActive = false;
        currentP.isGrounded = false;

        float rotY = currentT.rotation.y;
        float jumpOutSpeed = 10.0f;
        currentP.velocity.x = sinf(rotY) * jumpOutSpeed;
        currentP.velocity.z = cosf(rotY) * jumpOutSpeed;
        currentP.velocity.y = 15.0f; // 上へ

        // 2. 登場 (上空から)
        nextP.isActive = true;
        nextP.isGrounded = false;

        float spawnHeight = 15.0f;
        float spawnBehind = -3.0f;

        nextT.position.x = currentT.position.x + sinf(rotY) * spawnBehind;
        nextT.position.z = currentT.position.z + cosf(rotY) * spawnBehind;
        nextT.position.y = currentT.position.y + spawnHeight;
        nextT.rotation.y = currentT.rotation.y;

        nextP.velocity.x = 0.0f;
        nextP.velocity.z = 0.0f;
        nextP.velocity.y = -20.0f; // 急降下

        // 3. カメラ切り替え
        for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
            if (registry->HasComponent<CameraComponent>(id)) {
                auto& cam = registry->GetComponent<CameraComponent>(id);
                cam.targetEntityID = nextID;
                break;
            }
        }
        DebugLog("Switched! P%d(Exit) -> P%d(Enter)", currentID, nextID);
    }
}