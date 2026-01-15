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
#include "Game/EntityFactory.h"
#include "App/Main.h"
#include <DirectXMath.h>
#include <vector>
#include <cmath>
#include <limits>

using namespace DirectX;

void EnemySystem::Update(float dt) {
    auto registry = pWorld->GetRegistry();

    // 1. 全敵エンティティのループ
    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (!registry->HasComponent<EnemyComponent>(id)) continue;
        if (!registry->HasComponent<TransformComponent>(id)) continue;

        // 死んでいる敵は動かない（念のためチェック）
        if (registry->HasComponent<StatusComponent>(id)) {
            if (registry->GetComponent<StatusComponent>(id).hp <= 0) continue;
        }

        auto& enemy = registry->GetComponent<EnemyComponent>(id);
        auto& trans = registry->GetComponent<TransformComponent>(id);
        //// ★追加: 遠距離タイプ判定 (簡易的にHPが高い敵を遠距離タイプにする)
        //if (registry->HasComponent<StatusComponent>(id)) {
        //    if (registry->GetComponent<StatusComponent>(id).maxHp > 50) {
        //        enemy.isRanged = true;
        //    }
        //}
        // ---------------------------------------------------------
        // ターゲット（生存している最寄りのプレイヤー）を探す
        // ---------------------------------------------------------
        EntityID targetID = ECSConfig::INVALID_ID;
        float minDistSq = std::numeric_limits<float>::max(); // 無限大で初期化
        XMVECTOR enemyPos = XMLoadFloat3(&trans.position);

        for (EntityID pID = 0; pID < ECSConfig::MAX_ENTITIES; ++pID) {
            if (!registry->HasComponent<PlayerComponent>(pID)) continue;

            // 死んでいるプレイヤーは無視
            if (registry->HasComponent<StatusComponent>(pID)) {
                if (registry->GetComponent<StatusComponent>(pID).hp <= 0) continue;
            }
            auto& pComp = registry->GetComponent<PlayerComponent>(pID);
            if (!pComp.isActive) continue;

            auto& pTrans = registry->GetComponent<TransformComponent>(pID);
            XMVECTOR pPos = XMLoadFloat3(&pTrans.position);
            XMVECTOR diff = pPos - enemyPos;
            float distSq = XMVectorGetX(XMVector3LengthSq(diff));

            if (distSq < minDistSq) {
                minDistSq = distSq;
                targetID = pID;
            }
        }

        // ターゲットが見つからない（全滅など）なら何もしない
        if (targetID == ECSConfig::INVALID_ID) return;
        auto& targetTrans = registry->GetComponent<TransformComponent>(targetID);
        XMVECTOR targetPos = XMLoadFloat3(&targetTrans.position);
        float dist = std::sqrt(minDistSq);

        // ---------------------------------------------------------
        // ★追加: 遠距離攻撃ロジック (状態マシンとは並行して動く)
        // ---------------------------------------------------------
        if (enemy.isRanged) {
            // クールダウン更新
            if (enemy.attackCooldownTimer > 0.0f) {
                enemy.attackCooldownTimer -= dt;
            }

            // 射程内(20m) かつ クールダウン完了なら発射
            if (dist < 20.0f && enemy.attackCooldownTimer <= 0.0f) {
                // 方向計算
                float dx = targetTrans.position.x - trans.position.x;
                float dz = targetTrans.position.z - trans.position.z;
                float dLen = sqrt(dx * dx + dz * dz);

                if (dLen > 0.001f) {
                    XMFLOAT3 dir = { dx / dLen, 0.0f, dz / dLen }; // 水平撃ち

                    // 発射位置
                    XMFLOAT3 spawnPos = trans.position;
                    spawnPos.y += 1.0f;
                    spawnPos.x += dir.x * 1.5f;
                    spawnPos.z += dir.z * 1.5f;

                    int dmg = 10;
                    if (registry->HasComponent<StatusComponent>(id)) {
                        dmg = registry->GetComponent<StatusComponent>(id).attackPower;
                    }

                    // 弾生成
                    EntityFactory::CreateEnemyBullet(pWorld, spawnPos, dir, dmg);

                    // タイマーリセット
                    enemy.attackCooldownTimer = enemy.attackInterval;
                }
            }
        }

        // ---------------------------------------------------------
        // 状態マシン (State Machine)
        // ---------------------------------------------------------
        switch (enemy.state) {

            // --- [追跡状態] ---
        case EnemyState::Chase: {
            // 攻撃射程に入ったら攻撃開始
            if (!enemy.isRanged && dist <= enemy.attackRange) {
                enemy.state = EnemyState::Attack;
                enemy.attackTimer = enemy.attackDuration;

                DebugLog("Enemy(%d): Melee Attack Start!", id);

                int dmg = 10;
                if (registry->HasComponent<StatusComponent>(id)) {
                    dmg = registry->GetComponent<StatusComponent>(id).attackPower;
                }

                // 近接攻撃 (広がる球) を生成
                EntityFactory::CreateAttackSphere(pWorld, id, trans.position, dmg);
            }
            else {
                // --- 移動処理 ---
                float currentSpeed = enemy.moveSpeed;

                // 遠距離タイプの場合、近づきすぎたら止まる（または逃げる）
                if (enemy.isRanged) {
                    if (dist < 8.0f) {
                        currentSpeed = 0.0f; // 射撃位置を維持
                        // 逃げる処理を入れるなら: currentSpeed = -enemy.moveSpeed;
                    }
                }

                if (dist > 0.1f && currentSpeed != 0.0f) {
                    XMVECTOR dir = XMVector3Normalize(targetPos - enemyPos);

                    // 向き変更
                    float angle = atan2f(XMVectorGetX(dir), XMVectorGetZ(dir));
                    trans.rotation.y = angle;

                    // 移動
                    XMVECTOR vel = dir * currentSpeed * dt;
                    trans.position.x += XMVectorGetX(vel);
                    trans.position.z += XMVectorGetZ(vel);
                }
                else {
                    // 止まっていても向きだけは合わせる
                    XMVECTOR dir = XMVector3Normalize(targetPos - enemyPos);
                    float angle = atan2f(XMVectorGetX(dir), XMVectorGetZ(dir));
                    trans.rotation.y = angle;
                }
            }
            break;
        }

         // --- [攻撃状態] ---
        case EnemyState::Attack: {
            // 立ち止まって硬直
            enemy.attackTimer -= dt;
            if (enemy.attackTimer <= 0.0f) {
                // 攻撃終了 -> クールダウンへ
                enemy.state = EnemyState::Cooldown;
                enemy.attackTimer = enemy.cooldownTime;
                // ★追加: 攻撃終了ログ
                DebugLog("Enemy(%d): Attack End -> Cooldown", id);
            }
            break;
        }

        // --- [クールダウン状態] ---
        case EnemyState::Cooldown: {
            // 棒立ち、または少し後退するなど
            enemy.attackTimer -= dt;
            if (enemy.attackTimer <= 0.0f) {
                // 復帰 -> 再び追跡へ
                enemy.state = EnemyState::Chase;
                // ★追加: 復帰ログ
                DebugLog("Enemy(%d): Cooldown End -> Chase", id);
            }
            break;
        }
        }
    }
}