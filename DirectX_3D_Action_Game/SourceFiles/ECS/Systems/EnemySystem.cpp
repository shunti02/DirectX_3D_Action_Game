#define NOMINMAX
#include "ECS/Systems/EnemySystem.h"
#include "ECS/World.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/EnemyComponent.h"
#include "ECS/Components/StatusComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include "ECS/Components/BulletComponent.h" 
#include "Engine/AnimationManager.h"
#include "Game/EntityFactory.h"
#include "App/Main.h"
#include <DirectXMath.h>
#include <vector>
#include <cmath>
#include <limits>
#include <algorithm>

using namespace DirectX;

float DistSq(const XMFLOAT3& a, const XMFLOAT3& b) {
    float dx = a.x - b.x;
    float dz = a.z - b.z;
    return dx * dx + dz * dz;
}

void EnemySystem::Update(float dt) {
    timeAccumulator += dt;
    auto registry = pWorld->GetRegistry();

    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (!registry->HasComponent<EnemyComponent>(id)) continue;
        if (!registry->HasComponent<TransformComponent>(id)) continue;
        if (!registry->HasComponent<PhysicsComponent>(id)) continue;

        if (registry->HasComponent<StatusComponent>(id)) {
            if (registry->GetComponent<StatusComponent>(id).hp <= 0) continue;
        }

        auto& enemy = registry->GetComponent<EnemyComponent>(id);
        auto& trans = registry->GetComponent<TransformComponent>(id);
        auto& phy = registry->GetComponent<PhysicsComponent>(id);

        if (enemy.knockbackTimer > 0.0f) {
            enemy.knockbackTimer -= dt;
            enemy.state = EnemyState::Stun;
            if (std::abs(phy.velocity.y) < 0.1f) {
                phy.velocity.x *= 0.8f;
                phy.velocity.z *= 0.8f;
            }
            continue;
        }
        else if (enemy.state == EnemyState::Stun) {
            enemy.state = EnemyState::Chase;
        }

        EntityID targetID = ECSConfig::INVALID_ID;
        float minDistSq = std::numeric_limits<float>::max();
        XMVECTOR enemyPos = XMLoadFloat3(&trans.position);

        XMVECTOR targetPosVec = enemyPos;
        XMVECTOR targetVelVec = XMVectorZero();
        XMFLOAT3 targetPosF = trans.position;

        for (EntityID pID = 0; pID < ECSConfig::MAX_ENTITIES; ++pID) {
            if (!registry->HasComponent<PlayerComponent>(pID)) continue;
            auto& pStat = registry->GetComponent<StatusComponent>(pID);
            auto& pComp = registry->GetComponent<PlayerComponent>(pID);

            if (pStat.hp > 0 && pComp.isActive) {
                auto& pTrans = registry->GetComponent<TransformComponent>(pID);
                float d = DistSq(trans.position, pTrans.position);
                if (d < minDistSq) {
                    minDistSq = d;
                    targetID = pID;
                    targetPosVec = XMLoadFloat3(&pTrans.position);
                    targetPosF = pTrans.position;
                    if (registry->HasComponent<PhysicsComponent>(pID)) {
                        targetVelVec = XMLoadFloat3(&registry->GetComponent<PhysicsComponent>(pID).velocity);
                    }
                }
            }
        }

        if (targetID == ECSConfig::INVALID_ID) continue;
        float distToTarget = std::sqrt(minDistSq);

        bool isDodging = false;
        if (enemy.type != EnemyType::Heavy && enemy.thinkInterval <= 0.0f && enemy.state != EnemyState::Attack) {
            for (EntityID bID = 0; bID < ECSConfig::MAX_ENTITIES; ++bID) {
                if (!registry->HasComponent<BulletComponent>(bID)) continue;
                auto& bullet = registry->GetComponent<BulletComponent>(bID);
                if (!bullet.isActive || !bullet.fromPlayer) continue;

                auto& bTrans = registry->GetComponent<TransformComponent>(bID);
                auto& bPhy = registry->GetComponent<PhysicsComponent>(bID);
                XMVECTOR bPos = XMLoadFloat3(&bTrans.position);
                XMVECTOR bVel = XMLoadFloat3(&bPhy.velocity);

                XMVECTOR toEnemy = enemyPos - bPos;
                float bDist = XMVectorGetX(XMVector3Length(toEnemy));

                if (bDist < 15.0f && XMVectorGetX(XMVector3LengthSq(bVel)) > 1.0f) {
                    XMVECTOR bDir = XMVector3Normalize(bVel);
                    XMVECTOR toEnemyDir = XMVector3Normalize(toEnemy);
                    if (XMVectorGetX(XMVector3Dot(bDir, toEnemyDir)) > 0.95f) {
                        enemy.state = EnemyState::Strafing;
                        enemy.strafeDirection = (rand() % 2 == 0) ? 1.0f : -1.0f;
                        enemy.stateTimer = 0.4f;
                        enemy.thinkInterval = 1.0f;
                        isDodging = true;
                        break;
                    }
                }
            }
        }

        enemy.thinkInterval -= dt;
        if (!isDodging && enemy.thinkInterval <= 0.0f && enemy.state != EnemyState::Attack && enemy.state != EnemyState::Cooldown) {
            enemy.thinkInterval = 0.2f + (rand() % 20) / 100.0f;

            if (enemy.type == EnemyType::Heavy) {
                enemy.state = EnemyState::Chase;
            }
            else if (enemy.type == EnemyType::Ranged) {
                if (distToTarget < enemy.optimalRange * 0.8f) enemy.state = EnemyState::Retreat;
                else if (distToTarget > enemy.optimalRange * 2.0f) enemy.state = EnemyState::Chase;
                else {
                    enemy.state = EnemyState::Strafing;
                    enemy.strafeDirection = (rand() % 2 == 0) ? 1.0f : -1.0f;
                    enemy.stateTimer = 0.8f + (rand() % 10) / 10.0f;
                }
            }
            else if (enemy.type == EnemyType::Normal) {
                enemy.state = EnemyState::Chase;
            }
        }

        //‰“‹——£UŒ‚
        if ((enemy.type == EnemyType::Ranged || enemy.type == EnemyType::Heavy)) {
            if (enemy.attackCooldownTimer > 0.0f) enemy.attackCooldownTimer -= dt;

            float shootRange = (enemy.type == EnemyType::Ranged) ? 40.0f : 50.0f;

            if (enemy.state != EnemyState::Attack && enemy.state != EnemyState::Cooldown && distToTarget < shootRange && enemy.attackCooldownTimer <= 0.0f) {
                enemy.state = EnemyState::Attack;

                float duration = AnimationManager::GetInstance()->GetDuration("ShootRight");
                if (duration <= 0.0f) duration = 0.5f;
                enemy.attackDuration = duration;
                enemy.attackTimer = duration;

                enemy.attackCooldownTimer = 1.0f + (rand() % 6) / 10.0f;

                XMFLOAT3 spawnPos = trans.position;
                spawnPos.y += (enemy.type == EnemyType::Heavy) ? 0.8f : 1.0f;

                float bulletSpeed = 25.0f;
                float timeToHit = distToTarget / bulletSpeed;
                XMVECTOR predictedPos = targetPosVec + (targetVelVec * timeToHit * 0.85f);
                predictedPos = XMVectorSetY(predictedPos, XMVectorGetY(targetPosVec) + 1.0f);

                XMVECTOR startV = XMLoadFloat3(&spawnPos);
                XMVECTOR dirV = XMVector3Normalize(predictedPos - startV);
                XMFLOAT3 dir; XMStoreFloat3(&dir, dirV);

                int dmg = registry->HasComponent<StatusComponent>(id) ? registry->GetComponent<StatusComponent>(id).attackPower : 15;
                EntityFactory::CreateEnemyBullet(pWorld, spawnPos, dir, dmg);
                if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_SWITCH");
            }
        }

        XMVECTOR moveDir = XMVectorZero();
        float currentMoveSpeed = 0.0f;

        switch (enemy.state) {
        case EnemyState::Chase:
            if (enemy.type == EnemyType::Heavy) break;

            if (enemy.type == EnemyType::Normal && distToTarget <= enemy.attackRange) {
                enemy.state = EnemyState::Attack;

                float duration = AnimationManager::GetInstance()->GetDuration("AttackRight");
                if (duration <= 0.0f) duration = 1.0f;
                enemy.attackDuration = duration;
                enemy.attackTimer = duration;

                XMVECTOR atkPosV = enemyPos + XMVector3Normalize(targetPosVec - enemyPos) * 1.5f;
                atkPosV = XMVectorSetY(atkPosV, XMVectorGetY(atkPosV) + 1.0f);
                XMFLOAT3 atkPos; XMStoreFloat3(&atkPos, atkPosV);

                int dmg = registry->HasComponent<StatusComponent>(id) ? registry->GetComponent<StatusComponent>(id).attackPower : 20;
                EntityFactory::CreateAttackSphere(pWorld, id, atkPos, dmg);
            }
            else {
                moveDir = XMVector3Normalize(targetPosVec - enemyPos);
                currentMoveSpeed = enemy.moveSpeed * 1.5f;
            }
            break;

        case EnemyState::Strafing:
            enemy.stateTimer -= dt;
            if (enemy.stateTimer <= 0.0f) enemy.state = EnemyState::Chase;
            {
                XMVECTOR toTarget = XMVector3Normalize(targetPosVec - enemyPos);
                XMMATRIX rotMat = XMMatrixRotationY(XM_PIDIV2 * enemy.strafeDirection);
                moveDir = XMVector3TransformNormal(toTarget, rotMat);
                currentMoveSpeed = (enemy.stateTimer <= 0.4f) ? enemy.moveSpeed * 3.0f : enemy.moveSpeed * 1.2f;
            }
            break;

        case EnemyState::Retreat:
            if (distToTarget > enemy.optimalRange) enemy.state = EnemyState::Chase;
            moveDir = XMVector3Normalize(enemyPos - targetPosVec);
            currentMoveSpeed = enemy.moveSpeed * 1.5f;
            break;

        case EnemyState::Attack:
            currentMoveSpeed = 0.0f;
            enemy.attackTimer -= dt;
            if (enemy.attackTimer <= 0.0f) {
                enemy.state = EnemyState::Cooldown;
                enemy.attackTimer = enemy.cooldownTime;
            }
            break;

        case EnemyState::Cooldown:
            currentMoveSpeed = 0.0f;
            enemy.attackTimer -= dt;
            if (enemy.attackTimer <= 0.0f) {
                enemy.state = EnemyState::Chase;
            }
            break;
        }

        //•¨—‘¬“x‚Ì“K—p
        if (enemy.type != EnemyType::Heavy) {
            float targetVx = XMVectorGetX(moveDir) * currentMoveSpeed;
            float targetVz = XMVectorGetZ(moveDir) * currentMoveSpeed;

            float accel = (currentMoveSpeed > 0.0f) ? 30.0f : 40.0f;

            XMVECTOR separation = XMVectorZero();
            for (EntityID otherID = 0; otherID < ECSConfig::MAX_ENTITIES; ++otherID) {
                if (id == otherID) continue;
                if (!registry->HasComponent<EnemyComponent>(otherID)) continue;
                auto& otherTrans = registry->GetComponent<TransformComponent>(otherID);
                float dSq = DistSq(trans.position, otherTrans.position);
                if (dSq < 6.0f && dSq > 0.001f) {
                    XMVECTOR away = enemyPos - XMLoadFloat3(&otherTrans.position);
                    separation = XMVectorAdd(separation, XMVector3Normalize(away) / dSq);
                }
            }
            if (currentMoveSpeed > 0.0f) {
                XMVECTOR sepVel = XMVector3Normalize(separation) * currentMoveSpeed * 0.8f;
                targetVx += XMVectorGetX(sepVel);
                targetVz += XMVectorGetZ(sepVel);
            }

            phy.velocity.x += (targetVx - phy.velocity.x) * (accel * dt);
            phy.velocity.z += (targetVz - phy.velocity.z) * (accel * dt);
        }

        float targetYaw = trans.rotation.y;
        if (enemy.type == EnemyType::Normal && currentMoveSpeed > 0.1f && enemy.state != EnemyState::Attack) {
            targetYaw = atan2f(phy.velocity.x, phy.velocity.z);
        }
        else {
            XMVECTOR dirToTarget = XMVector3Normalize(targetPosVec - enemyPos);
            targetYaw = atan2f(XMVectorGetX(dirToTarget), XMVectorGetZ(dirToTarget));
        }

        float diff = targetYaw - trans.rotation.y;
        while (diff > XM_PI) diff -= XM_2PI;
        while (diff < -XM_PI) diff += XM_2PI;
        trans.rotation.y += diff * 20.0f * dt;
    }
}