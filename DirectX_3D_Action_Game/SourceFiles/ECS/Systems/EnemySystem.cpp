#include "ECS/Systems/EnemySystem.h" // パスは環境に合わせて調整してください

#include "ECS/World.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/EnemyComponent.h"
// #include "App/Game.h" // 今回のコードでは未使用のようですが、必要であれば残してください

#include <DirectXMath.h>
#include <cmath>

using namespace DirectX;

void EnemySystem::Update(float dt) {
    auto registry = pWorld->GetRegistry();

    // ターゲット（プレイヤー）を探す
    EntityID playerID = ECSConfig::INVALID_ID;
    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (registry->HasComponent<PlayerComponent>(id)) {
            playerID = id;
            break;
        }
    }

    // プレイヤーがいなければ何もしない
    if (playerID == ECSConfig::INVALID_ID) return;

    // プレイヤーの位置を取得
    auto& pTrans = registry->GetComponent<TransformComponent>(playerID);
    XMVECTOR targetPos = XMLoadFloat3(&pTrans.position);

    // 全ての敵に対して処理を行う
    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        // EnemyComponentを持っていなければスキップ
        if (!registry->HasComponent<EnemyComponent>(id)) continue;

        auto& eTrans = registry->GetComponent<TransformComponent>(id);
        auto& eComp = registry->GetComponent<EnemyComponent>(id);

        XMVECTOR enemyPos = XMLoadFloat3(&eTrans.position);

        // ベクトル計算
        XMVECTOR diff = targetPos - enemyPos;

        // 平面移動させる（Y軸の差を無視）
        diff = XMVectorSetY(diff, 0.0f);

        // 距離の二乗を計算
        float distSq = XMVectorGetX(XMVector3LengthSq(diff));
        float stopDistSq = eComp.stopDistance * eComp.stopDistance;

        // 停止位置より遠ければ近づく
        if (distSq > stopDistSq) {
            // 正規化
            XMVECTOR dir = XMVector3Normalize(diff);

            // 移動量計算
            XMVECTOR velocity = dir * eComp.moveSpeed * dt;
            XMVECTOR newPos = enemyPos + velocity;
            XMStoreFloat3(&eTrans.position, newPos);

            // 向きの制御 (atan2fでY軸回転を計算)
            float angle = atan2f(XMVectorGetX(dir), XMVectorGetZ(dir));

            // Y軸回転を適用
            eTrans.rotation.y = angle;
        }
    }
}