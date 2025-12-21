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


        // ---------------------------------------------------------
        // 状態マシン (State Machine)
        // ---------------------------------------------------------
        auto& targetTrans = registry->GetComponent<TransformComponent>(targetID);
        XMVECTOR targetPos = XMLoadFloat3(&targetTrans.position);
        float dist = std::sqrt(minDistSq);

        switch (enemy.state) {

            // --- [追跡状態] ---
        case EnemyState::Chase: {
            // 攻撃射程に入ったら攻撃開始
            if (dist <= enemy.attackRange) {
                enemy.state = EnemyState::Attack;
                enemy.attackTimer = enemy.attackDuration;

                // ★追加: 攻撃開始ログ
                DebugLog("Enemy(%d): Attack Start! Target:%d", id, targetID);

                // 攻撃発生！（AttackBox生成）
                // 敵の前方に判定を出す
                float angle = trans.rotation.y;
                float offsetDist = 1.5f;
                XMFLOAT3 spawnPos = {
                    trans.position.x + sinf(angle) * offsetDist,
                    trans.position.y + 0.5f,
                    trans.position.z + cosf(angle) * offsetDist
                };

                // 敵の攻撃力（Statusがあれば使う）
                int dmg = 10;
                if (registry->HasComponent<StatusComponent>(id)) {
                    dmg = registry->GetComponent<StatusComponent>(id).attackPower;
                }

                // EntityFactoryを使って攻撃判定を生成
                // ※敵の攻撃は少し大きめにするなどの調整もここで可能
                EntityFactory::CreateAttackHitbox(pWorld, id, spawnPos, { 2.0f, 2.0f, 2.0f }, dmg);
            }
            else {
                // 移動処理
                XMVECTOR dir = XMVector3Normalize(targetPos - enemyPos);

                // 向きを変える (Y軸回転)
                float angle = atan2f(XMVectorGetX(dir), XMVectorGetZ(dir));
                trans.rotation.y = angle;

                // 前進
                XMVECTOR vel = dir * enemy.moveSpeed * dt;
                // Y軸の移動は無視（重力はPhysicsSystem等でやるべきだが、簡易的にここでは高さ維持）
                // 本格的にはPhysicsSystemで速度加算する形が良いが、今は直接座標更新
                trans.position.x += XMVectorGetX(vel);
                trans.position.z += XMVectorGetZ(vel);
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