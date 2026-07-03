#include "ECS/Systems/PlayerSystem.h"
#include "ECS/World.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/StatusComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include "ECS/Components/CameraComponent.h"
#include "App/Game.h"
#include <cmath>
#include <algorithm>

using namespace DirectX;

void PlayerSystem::Update(float dt) {
    auto registry = pWorld->GetRegistry();
    Input* input = Game::GetInstance()->GetInput();

    float cameraYaw = 0.0f;
    for (EntityID camID = 0; camID < ECSConfig::MAX_ENTITIES; ++camID) {
        if (registry->HasComponent<CameraComponent>(camID)) {
            auto& cam = registry->GetComponent<CameraComponent>(camID);
            cameraYaw = cam.angleY;
            break;
        }
    }

    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (!registry->HasComponent<PlayerComponent>(id)) continue;
        if (!registry->HasComponent<PhysicsComponent>(id)) continue;

        auto& pComp = registry->GetComponent<PlayerComponent>(id);
        auto& trans = registry->GetComponent<TransformComponent>(id);
        auto& phy = registry->GetComponent<PhysicsComponent>(id);

        //Ž€–S”»’è
        if (registry->HasComponent<StatusComponent>(id)) {
            auto& status = registry->GetComponent<StatusComponent>(id);
            if (status.hp <= 0 && !pComp.isDead) {
                pComp.isDead = true;
                pComp.currentActionType = 0;
                phy.velocity.x = 0.0f;
                phy.velocity.z = 0.0f;
                pComp.velocity = { 0.0f, 0.0f, 0.0f };
                if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_EXPLOSION");
            }
        }

        if (pComp.isDead) continue;
        XMVECTOR moveDir = XMVectorZero();

        XMVECTOR forward = XMVectorSet(sinf(cameraYaw), 0.0f, cosf(cameraYaw), 0.0f);
        XMVECTOR right = XMVectorSet(cosf(cameraYaw), 0.0f, -sinf(cameraYaw), 0.0f);

        if (input->IsKey('W')) moveDir = XMVectorAdd(moveDir, forward);
        if (input->IsKey('S')) moveDir = XMVectorAdd(moveDir, XMVectorSet(-XMVectorGetX(forward), 0, -XMVectorGetZ(forward), 0));
        if (input->IsKey('D')) moveDir = XMVectorAdd(moveDir, right);
        if (input->IsKey('A')) moveDir = XMVectorAdd(moveDir, XMVectorSet(-XMVectorGetX(right), 0, -XMVectorGetZ(right), 0));

        pComp.isDashing = input->IsKey(VK_LSHIFT);

        float currentMaxSpeed = pComp.isDashing ? pComp.dashSpeed : pComp.moveSpeed;
        float currentMoveSpeed = pComp.isDashing ? pComp.dashSpeed : pComp.moveSpeed;

        if (pComp.currentActionType == 2 || pComp.currentActionType == 4) {
            currentMaxSpeed *= 0.2f;
            currentMoveSpeed *= 0.2f;
        }
        else if (pComp.currentActionType == 1 || pComp.currentActionType == 3) {
            currentMaxSpeed *= 0.9f;
            currentMoveSpeed *= 0.9f;
        }

        //ˆÚ“®ŒvŽZ
        if (!XMVector3Equal(moveDir, XMVectorZero())) {
            moveDir = XMVector3Normalize(moveDir);

            float targetVx = XMVectorGetX(moveDir) * currentMoveSpeed;
            float targetVz = XMVectorGetZ(moveDir) * currentMoveSpeed;

            float accel = pComp.isGrounded ? pComp.acceleration : (pComp.acceleration * 0.2f);
            phy.velocity.x += (targetVx - phy.velocity.x) * (accel * dt);
            phy.velocity.z += (targetVz - phy.velocity.z) * (accel * dt);

            if (pComp.currentActionType == 0) {
                float targetYaw = atan2f(phy.velocity.x, phy.velocity.z);
                float diff = targetYaw - trans.rotation.y;
                while (diff > DirectX::XM_PI) diff -= DirectX::XM_2PI;
                while (diff < -DirectX::XM_PI) diff += DirectX::XM_2PI;

                trans.rotation.y += diff * (pComp.turnSpeed * dt);
            }
        }
        else {
            float friction = pComp.isGrounded ? pComp.deceleration : (pComp.deceleration * 0.1f);
            phy.velocity.x += (0.0f - phy.velocity.x) * (friction * dt);
            phy.velocity.z += (0.0f - phy.velocity.z) * (friction * dt);

            if (std::abs(phy.velocity.x) < 0.1f) phy.velocity.x = 0.0f;
            if (std::abs(phy.velocity.z) < 0.1f) phy.velocity.z = 0.0f;
        }

        if (pComp.currentActionType > 0) {
            float targetYaw = cameraYaw;
            float diff = targetYaw - trans.rotation.y;
            while (diff > DirectX::XM_PI) diff -= DirectX::XM_2PI;
            while (diff < -DirectX::XM_PI) diff += DirectX::XM_2PI;

            trans.rotation.y += diff * (pComp.turnSpeed * 2.0f * dt);
        }

        pComp.velocity.x = phy.velocity.x;
        pComp.velocity.z = phy.velocity.z;

        //ƒWƒƒƒ“ƒvˆ—
        if (input->IsKeyDown(VK_SPACE) && pComp.isGrounded) {
            phy.velocity.y = pComp.jumpPower;
            pComp.velocity.y = pComp.jumpPower;
            pComp.isGrounded = false;
            if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_JUMP");
        }
    }
}