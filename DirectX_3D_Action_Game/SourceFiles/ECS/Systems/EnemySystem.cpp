/*===================================================================
// ファイル: EnemySystem.cpp
// 概要: 敵のAI（追跡・攻撃・クールダウン）を制御するシステム
=====================================================================*/
#define NOMINMAX
#include "ECS/Systems/EnemySystem.h"
#include "ECS/World.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/EnemyComponent.h"
#include "ECS/Components/StatusComponent.h"
#include "ECS/Components/PhysicsComponent.h"
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

        if (registry->HasComponent<StatusComponent>(id)) {
            if (registry->GetComponent<StatusComponent>(id).hp <= 0) continue;
        }

        auto& enemy = registry->GetComponent<EnemyComponent>(id);
        auto& trans = registry->GetComponent<TransformComponent>(id);

        // 0. ノックバック・スタン
        if (enemy.knockbackTimer > 0.0f) {
            enemy.knockbackTimer -= dt;
            enemy.state = EnemyState::Stun;
            if (registry->HasComponent<PhysicsComponent>(id)) {
                auto& phy = registry->GetComponent<PhysicsComponent>(id);
                if (std::abs(phy.velocity.y) < 0.1f) {
                    phy.velocity.x *= 0.9f;
                    phy.velocity.z *= 0.9f;
                }
            }
            continue;
        }
        else if (enemy.state == EnemyState::Stun) {
            enemy.state = (enemy.type == EnemyType::Boss) ? EnemyState::BossIdle : EnemyState::Chase;
        }

        // 1. ターゲット検索
        EntityID targetID = ECSConfig::INVALID_ID;
        float minDistSq = std::numeric_limits<float>::max();
        XMVECTOR enemyPos = XMLoadFloat3(&trans.position);

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
                }
            }
        }

        if (targetID == ECSConfig::INVALID_ID && enemy.type != EnemyType::Boss) return;

        // ★変数名を統一 (distToTarget, targetPosVec)
        float distToTarget = 0.0f;
        XMVECTOR targetPosVec = enemyPos; // いない場合は自分の位置
        XMFLOAT3 targetPosF = trans.position;

        if (targetID != ECSConfig::INVALID_ID) {
            auto& targetTrans = registry->GetComponent<TransformComponent>(targetID);
            targetPosVec = XMLoadFloat3(&targetTrans.position);
            targetPosF = targetTrans.position;
            distToTarget = std::sqrt(minDistSq);
        }

        // ---------------------------------------------------------
        // ★ボス専用AI
        // ---------------------------------------------------------
        if (enemy.type == EnemyType::Boss) {
            enemy.attackCooldownTimer -= dt;

            if (registry->HasComponent<StatusComponent>(id)) {
                auto& st = registry->GetComponent<StatusComponent>(id);
                if (st.hp < st.maxHp / 2) enemy.bossPhase = 2;
            }

            if (enemy.state != EnemyState::BossIdle &&
                enemy.state != EnemyState::BossRingBarrage &&
                enemy.state != EnemyState::BossBitLaser &&
                enemy.state != EnemyState::BossRapidFire)
            {
                enemy.state = EnemyState::BossIdle;
            }

            if (enemy.state == EnemyState::BossIdle) {
                if (enemy.attackCooldownTimer <= 0.0f) {
                    int pattern = rand() % 100;
                    if (enemy.bossPhase == 1) {
                        if (pattern < 60) {
                            enemy.state = EnemyState::BossRingBarrage;
                            enemy.attackTimer = 3.0f;
                        }
                        else {
                            enemy.state = EnemyState::BossRapidFire;
                            enemy.attackTimer = 2.0f;
                        }
                    }
                    else {
                        if (pattern < 30) {
                            enemy.state = EnemyState::BossRingBarrage;
                            enemy.attackTimer = 3.0f;
                        }
                        else if (pattern < 60) {
                            enemy.state = EnemyState::BossRapidFire;
                            enemy.attackTimer = 1.5f;
                        }
                        else {
                            enemy.state = EnemyState::BossBitLaser;
                            enemy.attackTimer = 2.0f;
                        }
                    }
                    enemy.stateTimer = 0.0f;
                }
            }
            else if (enemy.state == EnemyState::BossRingBarrage) {
                enemy.attackTimer -= dt;
                trans.rotation.y += dt * 2.0f;
                enemy.stateTimer += dt;

                if (enemy.stateTimer > 0.1f) {
                    enemy.stateTimer = 0.0f;
                    XMFLOAT3 spawnPos = trans.position;
                    spawnPos.y += 1.0f;

                    float dirY = 0.0f;
                    if (targetID != ECSConfig::INVALID_ID) {
                        float targetY = targetPosF.y + 0.5f;
                        float dy = targetY - spawnPos.y;
                        // std::max のエラー回避: (std::max)(...)
                        float hDist = (std::max)(distToTarget, 0.1f);
                        dirY = dy / hDist;
                    }
                    else {
                        dirY = -0.2f;
                    }

                    for (int i = 0; i < 4; ++i) {
                        float angle = trans.rotation.y + (XM_PIDIV2 * i);
                        XMVECTOR vDir = XMVectorSet(sinf(angle), dirY, cosf(angle), 0.0f);
                        vDir = XMVector3Normalize(vDir);
                        XMFLOAT3 dir;
                        XMStoreFloat3(&dir, vDir);
                        EntityFactory::CreateEnemyBullet(pWorld, spawnPos, dir, 20);
                    }
                }
                if (enemy.attackTimer <= 0.0f) {
                    enemy.state = EnemyState::BossIdle;
                    enemy.attackCooldownTimer = (enemy.bossPhase == 2) ? 2.0f : 4.0f;
                }
            }
            else if (enemy.state == EnemyState::BossRapidFire) {
                enemy.attackTimer -= dt;
                enemy.stateTimer += dt;

                if (targetID != ECSConfig::INVALID_ID) {
                    XMVECTOR dir = XMVector3Normalize(targetPosVec - enemyPos);
                    float angle = atan2f(XMVectorGetX(dir), XMVectorGetZ(dir));
                    float diff = angle - trans.rotation.y;
                    while (diff > XM_PI) diff -= XM_2PI;
                    while (diff < -XM_PI) diff += XM_2PI;
                    trans.rotation.y += diff * 5.0f * dt;
                }

                float interval = (enemy.bossPhase == 2) ? 0.3f : 0.5f;
                if (enemy.stateTimer > interval) {
                    enemy.stateTimer = 0.0f;
                    if (targetID != ECSConfig::INVALID_ID) {
                        XMVECTOR pCore = XMLoadFloat3(&targetPosF);
                        pCore = XMVectorAdd(pCore, XMVectorSet(0, 0.5f, 0, 0));
                        XMFLOAT3 spawnPos = trans.position;
                        spawnPos.y += 1.0f;
                        XMVECTOR start = XMLoadFloat3(&spawnPos);
                        XMVECTOR dirV = XMVector3Normalize(pCore - start);
                        XMFLOAT3 dir;
                        XMStoreFloat3(&dir, dirV);
                        EntityFactory::CreateEnemyBullet(pWorld, spawnPos, dir, 30);
                    }
                }
                if (enemy.attackTimer <= 0.0f) {
                    enemy.state = EnemyState::BossIdle;
                    enemy.attackCooldownTimer = 3.0f;
                }
            }
            else if (enemy.state == EnemyState::BossBitLaser) {
                enemy.attackTimer -= dt;
                enemy.stateTimer += dt;

                if (enemy.stateTimer > 0.2f && targetID != ECSConfig::INVALID_ID) {
                    enemy.stateTimer = 0.0f;
                    XMVECTOR pCore = XMLoadFloat3(&targetPosF);
                    pCore = XMVectorAdd(pCore, XMVectorSet(0, 0.5f, 0, 0));
                    for (int i = 0; i < 4; ++i) {
                        float angle = (XM_2PI / 4.0f) * i + (timeAccumulator * 2.0f);
                        float r = 5.0f;
                        XMFLOAT3 bitPos = trans.position;
                        bitPos.x += cosf(angle) * r;
                        bitPos.z += sinf(angle) * r;
                        bitPos.y += 2.0f;
                        XMVECTOR start = XMLoadFloat3(&bitPos);
                        XMVECTOR dirV = XMVector3Normalize(pCore - start);
                        XMFLOAT3 dir;
                        XMStoreFloat3(&dir, dirV);
                        EntityFactory::CreateEnemyBullet(pWorld, bitPos, dir, 15);
                    }
                }
                if (enemy.attackTimer <= 0.0f) {
                    enemy.state = EnemyState::BossIdle;
                    enemy.attackCooldownTimer = 3.0f;
                }
            }
            continue;
        }

        // ---------------------------------------------------------
        // 2. 雑魚敵の思考
        // ---------------------------------------------------------
        enemy.thinkInterval -= dt;
        if (enemy.thinkInterval <= 0.0f && enemy.state != EnemyState::Attack && enemy.state != EnemyState::Cooldown) {
            enemy.thinkInterval = 0.5f + (rand() % 50) / 100.0f;

            if (enemy.isImmovable) {
                enemy.state = EnemyState::Chase;
            }
            else if (enemy.isRanged) {
                if (distToTarget < 8.0f) enemy.state = EnemyState::Retreat;
                else if (distToTarget > enemy.optimalRange + 5.0f) enemy.state = EnemyState::Chase;
                else {
                    if (rand() % 100 < 40) {
                        enemy.state = EnemyState::Strafing;
                        enemy.strafeDirection = (rand() % 2 == 0) ? 1.0f : -1.0f;
                        enemy.stateTimer = 1.0f;
                    }
                    else {
                        enemy.state = EnemyState::Chase;
                    }
                }
            }
            else {
                if (distToTarget < 10.0f && distToTarget > enemy.attackRange) {
                    if (rand() % 100 < 30) {
                        enemy.state = EnemyState::Strafing;
                        enemy.strafeDirection = (rand() % 2 == 0) ? 1.0f : -1.0f;
                        enemy.stateTimer = 0.8f;
                    }
                    else {
                        enemy.state = EnemyState::Chase;
                    }
                }
                else {
                    enemy.state = EnemyState::Chase;
                }
            }
        }

        // ---------------------------------------------------------
        // 3. 雑魚敵の遠距離攻撃
        // ---------------------------------------------------------
        if (enemy.isRanged) {
            if (enemy.attackCooldownTimer > 0.0f) enemy.attackCooldownTimer -= dt;

            if (distToTarget < 30.0f && enemy.attackCooldownTimer <= 0.0f) {
                XMFLOAT3 spawnPos = trans.position;
                spawnPos.y += 1.0f;
                XMFLOAT3 targetCorePos = targetPosF;
                targetCorePos.y += 0.5f;

                XMVECTOR startV = XMLoadFloat3(&spawnPos);
                XMVECTOR endV = XMLoadFloat3(&targetCorePos);
                XMVECTOR dirV = XMVector3Normalize(endV - startV);
                XMFLOAT3 dir;
                XMStoreFloat3(&dir, dirV);

                int dmg = 10;
                if (registry->HasComponent<StatusComponent>(id)) {
                    dmg = registry->GetComponent<StatusComponent>(id).attackPower;
                }
                EntityFactory::CreateEnemyBullet(pWorld, spawnPos, dir, dmg);
                enemy.attackCooldownTimer = enemy.attackInterval;
            }
        }

        // ---------------------------------------------------------
        // 4. 雑魚敵の行動実行
        // ---------------------------------------------------------
        XMVECTOR moveDir = XMVectorZero();
        float currentMoveSpeed = enemy.moveSpeed;

        switch (enemy.state) {
        case EnemyState::Chase:
            if (enemy.isImmovable) {
                XMVECTOR dir = XMVector3Normalize(targetPosVec - enemyPos);
                float angle = atan2f(XMVectorGetX(dir), XMVectorGetZ(dir));
                trans.rotation.y = angle;
                break;
            }
            if (!enemy.isRanged && distToTarget <= enemy.attackRange) {
                enemy.state = EnemyState::Attack;
                enemy.attackTimer = enemy.attackDuration;
                int dmg = 10;
                if (registry->HasComponent<StatusComponent>(id)) dmg = registry->GetComponent<StatusComponent>(id).attackPower;
                EntityFactory::CreateAttackSphere(pWorld, id, trans.position, dmg);
            }
            else {
                moveDir = XMVector3Normalize(targetPosVec - enemyPos);
                if (enemy.isRanged && distToTarget < enemy.optimalRange && distToTarget > 8.0f) currentMoveSpeed = 0.0f;
            }
            break;

        case EnemyState::Strafing:
            enemy.stateTimer -= dt;
            if (enemy.stateTimer <= 0.0f) enemy.state = EnemyState::Chase;
            {
                XMVECTOR toTarget = XMVector3Normalize(targetPosVec - enemyPos);
                XMMATRIX rotMat = XMMatrixRotationY(XM_PIDIV2 * enemy.strafeDirection);
                moveDir = XMVector3TransformNormal(toTarget, rotMat);
                if (!enemy.isRanged) moveDir = XMVectorAdd(moveDir, toTarget * 0.3f);
                moveDir = XMVector3Normalize(moveDir);
            }
            break;

        case EnemyState::Retreat:
            if (distToTarget > 12.0f) enemy.state = EnemyState::Chase;
            moveDir = XMVector3Normalize(enemyPos - targetPosVec);
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
            if (enemy.attackTimer <= 0.0f) enemy.state = EnemyState::Chase;
            break;
        }

        if (XMVectorGetX(XMVector3LengthSq(moveDir)) > 0.001f && currentMoveSpeed > 0.0f) {
            float angle = atan2f(XMVectorGetX(moveDir), XMVectorGetZ(moveDir));
            trans.rotation.y = angle;

            XMVECTOR separation = XMVectorZero();
            int neighborCount = 0;
            for (EntityID otherID = 0; otherID < ECSConfig::MAX_ENTITIES; ++otherID) {
                if (id == otherID) continue;
                if (!registry->HasComponent<EnemyComponent>(otherID)) continue;
                if (registry->HasComponent<StatusComponent>(otherID) && registry->GetComponent<StatusComponent>(otherID).hp <= 0) continue;

                auto& otherTrans = registry->GetComponent<TransformComponent>(otherID);
                float dSq = DistSq(trans.position, otherTrans.position);
                if (dSq < 4.0f) {
                    XMVECTOR away = enemyPos - XMLoadFloat3(&otherTrans.position);
                    separation = XMVectorAdd(separation, XMVector3Normalize(away) / (dSq + 0.1f));
                    neighborCount++;
                }
            }
            if (neighborCount > 0) {
                moveDir = XMVectorAdd(moveDir, separation * 1.5f);
                moveDir = XMVector3Normalize(moveDir);
            }

            XMVECTOR velocity = moveDir * currentMoveSpeed * dt;
            trans.position.x += XMVectorGetX(velocity);
            trans.position.z += XMVectorGetZ(velocity);
        }
    }
}