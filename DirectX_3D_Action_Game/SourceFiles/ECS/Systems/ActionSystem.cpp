#include "ECS/Systems/ActionSystem.h"
#include "ECS/World.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/ActionComponent.h"
#include "ECS/Components/StatusComponent.h"
#include "ECS/Components/AttackBoxComponent.h"
#include "ECS/Components/BulletComponent.h"
#include "ECS/Components/CameraComponent.h"
#include "ECS/Components/PlayerPartComponent.h"
#include "Game/EntityFactory.h"
#include "App/Game.h"
#include <DirectXMath.h>

using namespace DirectX;

void ActionSystem::Update(float dt) {
    auto registry = pWorld->GetRegistry();
    Input* input = Game::GetInstance()->GetInput();

    //攻撃判定(Hitbox/Bullet)の寿命管理
    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (registry->HasComponent<AttackBoxComponent>(id)) {
            auto& box = registry->GetComponent<AttackBoxComponent>(id);
            box.lifeTime -= dt;
            if (box.lifeTime <= 0.0f) pWorld->DestroyEntity(id);
        }
        if (registry->HasComponent<BulletComponent>(id)) {
            auto& bullet = registry->GetComponent<BulletComponent>(id);
            bullet.lifeTime -= dt;
            if (bullet.lifeTime <= 0.0f) pWorld->DestroyEntity(id);
        }
    }

    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (registry->HasComponent<AttackSphereComponent>(id)) {
            auto& sphere = registry->GetComponent<AttackSphereComponent>(id);
            sphere.lifeTime -= dt;
            if (sphere.lifeTime <= 0.0f) {
                pWorld->DestroyEntity(id);
                continue;
            }
        }
    }

    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (registry->HasComponent<AttackBoxComponent>(id)) {
            auto& box = registry->GetComponent<AttackBoxComponent>(id);
            box.lifeTime -= dt;
            if (box.lifeTime <= 0.0f) {
                pWorld->DestroyEntity(id);
            }
        }
    }

    //カメラ方向取得
    XMVECTOR camDir = XMVectorSet(0, 0, 1, 0);
    for (EntityID camID = 0; camID < ECSConfig::MAX_ENTITIES; ++camID) {
        if (registry->HasComponent<CameraComponent>(camID)) {
            auto& cam = registry->GetComponent<CameraComponent>(camID);
            XMMATRIX camRot = XMMatrixRotationRollPitchYaw(cam.angleX, cam.angleY, 0.0f);
            camDir = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), camRot);
            break;
        }
    }
    XMFLOAT3 dirF; XMStoreFloat3(&dirF, camDir);

    //プレイヤーが装備している武器IDを取得
    int weaponRightID = -1;
    int weaponLeftID = -1;
    for (EntityID pID = 0; pID < ECSConfig::MAX_ENTITIES; ++pID) {
        if (registry->HasComponent<PlayerPartComponent>(pID)) {
            auto& part = registry->GetComponent<PlayerPartComponent>(pID);
            if (part.partType == PartType::HandRight) weaponRightID = part.partModelID;
            else if (part.partType == PartType::HandLeft) weaponLeftID = part.partModelID;
        }
    }

    // プレイヤーのアクション入力処理
    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (!registry->HasComponent<PlayerComponent>(id) || !registry->HasComponent<ActionComponent>(id)) continue;

        auto& pComp = registry->GetComponent<PlayerComponent>(id);
        auto& trans = registry->GetComponent<TransformComponent>(id);
        auto& action = registry->GetComponent<ActionComponent>(id);
        auto& status = registry->GetComponent<StatusComponent>(id);

        if (pComp.isDead) continue;

        if (action.attackCooldown > 0.0f) action.attackCooldown -= dt;
        if (pComp.actionTimer > 0.0f) {
            pComp.actionTimer -= dt;
            if (pComp.actionTimer <= 0.0f) pComp.currentActionType = 0;
        }

        float angle = trans.rotation.y;
        XMVECTOR forward = XMVectorSet(sinf(angle), 0.0f, cosf(angle), 0.0f);
        XMVECTOR right = XMVectorSet(cosf(angle), 0.0f, -sinf(angle), 0.0f);

        //右手武器の攻撃
        if (input->IsMouseKeyDown(1) && action.attackCooldown <= 0.0f) {
            if (weaponRightID == 0 || weaponRightID == 1) {
                pComp.currentActionType = 1;
                pComp.actionTimer = 0.5f;
                action.attackCooldown = 0.6f;

                XMVECTOR spawnVec = XMLoadFloat3(&trans.position) + right * 1.0f + XMVectorSet(0, 1.0f, 0, 0);
                XMFLOAT3 spawnPos; XMStoreFloat3(&spawnPos, spawnVec);

                EntityFactory::CreatePlayerBullet(pWorld, spawnPos, dirF, status.attackPower);
                EntityFactory::CreateMuzzleFlash(pWorld, spawnPos, dirF);
                if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_SWITCH");
            }
            else if (weaponRightID == 2) { 
                pComp.currentActionType = 2;
                pComp.actionTimer = 1.0f;
                action.attackCooldown = 1.2f;

                XMVECTOR spawnVec = XMLoadFloat3(&trans.position) + forward * 5.0f + XMVectorSet(0, -0.5f, 0, 0);
                XMFLOAT3 spawnPos; XMStoreFloat3(&spawnPos, spawnVec);

                EntityFactory::CreateAttackHitbox(pWorld, id, spawnPos, { 4.0f, 4.0f, 4.0f }, status.attackPower * 2, 0.8f);
                if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_JUMP");
                if (registry->HasComponent<PhysicsComponent>(id)) {
                    auto& phy = registry->GetComponent<PhysicsComponent>(id);
                    phy.velocity.x += XMVectorGetX(forward) * 25.0f;
                    phy.velocity.z += XMVectorGetZ(forward) * 25.0f;
                }
            }
        }
        //左手武器の攻撃
        else if (input->IsMouseKeyDown(0) && action.attackCooldown <= 0.0f) {
            if (weaponLeftID == 0 || weaponLeftID == 1) {
                pComp.currentActionType = 3;
                pComp.actionTimer = 0.5f;
                action.attackCooldown = 0.6f;

                XMVECTOR spawnVec = XMLoadFloat3(&trans.position) - right * 1.0f + XMVectorSet(0, 1.0f, 0, 0);
                XMFLOAT3 spawnPos; XMStoreFloat3(&spawnPos, spawnVec);

                EntityFactory::CreatePlayerBullet(pWorld, spawnPos, dirF, status.attackPower);
                EntityFactory::CreateMuzzleFlash(pWorld, spawnPos, dirF);
                if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_SWITCH");
            }
            else if (weaponLeftID == 2) {
                pComp.currentActionType = 4;
                pComp.actionTimer = 1.0f;
                action.attackCooldown = 1.2f;

                XMVECTOR spawnVec = XMLoadFloat3(&trans.position) + forward * 5.0f + XMVectorSet(0, -0.5f, 0, 0);
                XMFLOAT3 spawnPos; XMStoreFloat3(&spawnPos, spawnVec);

                EntityFactory::CreateAttackHitbox(pWorld, id, spawnPos, { 4.0f, 4.0f, 4.0f }, status.attackPower * 2, 0.8f);
                if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_JUMP");
                if (registry->HasComponent<PhysicsComponent>(id)) {
                    auto& phy = registry->GetComponent<PhysicsComponent>(id);
                    phy.velocity.x += XMVectorGetX(forward) * 25.0f;
                    phy.velocity.z += XMVectorGetZ(forward) * 25.0f;
                }
            }
        }
    }
}