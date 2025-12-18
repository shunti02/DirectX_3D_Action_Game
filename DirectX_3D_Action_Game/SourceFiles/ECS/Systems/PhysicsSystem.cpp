#define NOMINMAX 

#include "App/Main.h"
#include "ECS/Systems/PhysicsSystem.h"
#include "ECS/World.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/ColliderComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/AttackBoxComponent.h"
#include "ECS/Components/RecoveryBoxComponent.h" // ★追加
#include "ECS/Components/Roles.h"
#include "ECS/Components/StatusComponent.h"
#include <vector>
#include <cmath>
#include <algorithm> // std::max, std::min
#include <DirectXMath.h>

using namespace DirectX;

// -----------------------------------------------------------------------
// 内部ヘルパー関数: 線分(p1-q1) と 線分(p2-q2) の最短距離の2乗を求める
// -----------------------------------------------------------------------
static float SegmentToSegmentDistSq(
    XMVECTOR p1, XMVECTOR q1,
    XMVECTOR p2, XMVECTOR q2,
    XMVECTOR& outC1, XMVECTOR& outC2)
{
    XMVECTOR d1 = q1 - p1;
    XMVECTOR d2 = q2 - p2;
    XMVECTOR r = p1 - p2;
    float a = XMVectorGetX(XMVector3Dot(d1, d1));
    float e = XMVectorGetX(XMVector3Dot(d2, d2));
    float f = XMVectorGetX(XMVector3Dot(d2, r));

    if (a <= 0.00001f && e <= 0.00001f) {
        outC1 = p1; outC2 = p2;
        return XMVectorGetX(XMVector3LengthSq(p1 - p2));
    }
    if (a <= 0.00001f) {
        outC1 = p1;
        float t = std::max(0.0f, std::min(1.0f, f / e));
        outC2 = p2 + d2 * t;
        return XMVectorGetX(XMVector3LengthSq(outC1 - outC2));
    }
    if (e <= 0.00001f) {
        outC2 = p2;
        float t = std::max(0.0f, std::min(1.0f, -XMVectorGetX(XMVector3Dot(d1, r)) / a));
        outC1 = p1 + d1 * t;
        return XMVectorGetX(XMVector3LengthSq(outC1 - outC2));
    }

    float c = XMVectorGetX(XMVector3Dot(d1, r));
    float b = XMVectorGetX(XMVector3Dot(d1, d2));
    float denom = a * e - b * b;

    float s, t;
    if (denom != 0.0f) {
        s = std::max(0.0f, std::min(1.0f, (b * f - c * e) / denom));
    }
    else {
        s = 0.0f;
    }

    t = (b * s + f) / e;
    if (t < 0.0f) {
        t = 0.0f;
        s = std::max(0.0f, std::min(1.0f, -c / a));
    }
    else if (t > 1.0f) {
        t = 1.0f;
        s = std::max(0.0f, std::min(1.0f, (b - c) / a));
    }

    outC1 = p1 + d1 * s;
    outC2 = p2 + d2 * t;
    return XMVectorGetX(XMVector3LengthSq(outC1 - outC2));
}


// -----------------------------------------------------------------------
// OBB取得関数の実装
// -----------------------------------------------------------------------
OBB PhysicsSystem::GetOBB(EntityID id) {
    auto registry = pWorld->GetRegistry();
    OBB obb = {};
    //初期化
    XMStoreFloat4x4(&obb.worldMatrix, XMMatrixIdentity());
    obb.center = { 0,0,0 };
	obb.extents = { 0.5f,0.5f,0.5f };

    if (!registry->HasComponent<TransformComponent>(id) ||
        !registry->HasComponent<ColliderComponent>(id)) {
        return obb;
    }

    const auto& trans = registry->GetComponent<TransformComponent>(id);
    const auto& col = registry->GetComponent<ColliderComponent>(id);

	//行列計算
    XMMATRIX R = XMMatrixRotationRollPitchYaw(trans.rotation.x, trans.rotation.y, trans.rotation.z);
    XMMATRIX T = XMMatrixTranslation(trans.position.x, trans.position.y, trans.position.z);
    XMMATRIX world = R * T;

    XMStoreFloat4x4(&obb.worldMatrix, world);

    // 2. 中心 (Transformの位置)
    obb.center = trans.position;

    if (col.type == ColliderType::Type_Box) {
        obb.extents = {
            col.size.x * trans.scale.x * 0.5f,
            col.size.y * trans.scale.y * 0.5f,
            col.size.z * trans.scale.z * 0.5f
        };
    }
    else {
        // カプセルの場合 (半径はXスケール、高さはYスケールに依存とみなす)
        float scaledRadius = col.radius * trans.scale.x;
        float scaledHeight = col.height * trans.scale.y;
        obb.extents = { scaledRadius, scaledHeight * 0.5f, scaledRadius };
    }

    return obb;
}
// -----------------------------------------------------------------------
// Update関数の実装
// -----------------------------------------------------------------------
void PhysicsSystem::Update(float dt) {
    auto registry = pWorld->GetRegistry();

    //無敵時間の更新
    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (registry->HasComponent<StatusComponent>(id)) {
            auto& status = registry->GetComponent<StatusComponent>(id);
            if (status.invincibleTimer > 0.0f) {
                status.invincibleTimer -= dt;
            }
        }
    }

    EntityID playerID = ECSConfig::INVALID_ID;
    for (EntityID playerID = 0; playerID < ECSConfig::MAX_ENTITIES; ++playerID) {
        if (!registry->HasComponent<PlayerComponent>(playerID))continue;
        if (!registry->HasComponent<ColliderComponent>(playerID))continue;
        //接地フラグをリセット
        auto& pComp = registry->GetComponent<PlayerComponent>(playerID);
        pComp.isGrounded = false;

        for (EntityID otherID = 0; otherID < ECSConfig::MAX_ENTITIES; ++otherID) {
            if (playerID == otherID) continue;
            if (!registry->HasComponent<ColliderComponent>(otherID)) continue;
            CheckAndResolve(playerID, otherID);
        }
    }

    // ---------------------------------------------------------
    // 攻撃判定のループ
    // ---------------------------------------------------------
    for (EntityID attackID = 0; attackID < ECSConfig::MAX_ENTITIES; ++attackID) {
        //AttackBoxComponentを持っていないならスキップ
        if (!registry->HasComponent<AttackBoxComponent>(attackID)) continue;

        //攻撃の持ち主を取得
        auto& attackBox = registry->GetComponent<AttackBoxComponent>(attackID);
        EntityID ownerID = attackBox.ownerID;

        //当たり対象を探す
        for (EntityID targetID = 0; targetID < ECSConfig::MAX_ENTITIES; ++targetID) {
            if (attackID == targetID) continue;
            if (targetID == ownerID) continue;//自分には当てない

            //コライダーとステータスを持っている相手だけ対象
            if (!registry->HasComponent<ColliderComponent>(targetID)) continue;
            if (!registry->HasComponent<StatusComponent>(targetID)) continue;

            //衝突判定＆ダメージ
            CheckAttackHit(attackID, targetID);
        }
    }
    // ---------------------------------------------------------
    // 4. ★追加: 回復判定のループ
    // ---------------------------------------------------------
    for (EntityID recoveryID = 0; recoveryID < ECSConfig::MAX_ENTITIES; ++recoveryID) {
        if (!registry->HasComponent<RecoveryBoxComponent>(recoveryID)) continue;

        auto& recBox = registry->GetComponent<RecoveryBoxComponent>(recoveryID);
        EntityID ownerID = recBox.ownerID;

        // 回復対象を探す (プレイヤーのみ対象とする場合)
        for (EntityID targetID = 0; targetID < ECSConfig::MAX_ENTITIES; ++targetID) {
            if (recoveryID == targetID) continue;

            // プレイヤーかつステータスを持っているもの
            if (!registry->HasComponent<PlayerComponent>(targetID)) continue;
            if (!registry->HasComponent<StatusComponent>(targetID)) continue;
            if (!registry->HasComponent<ColliderComponent>(targetID)) continue;

            // 判定＆回復
            CheckRecoveryHit(recoveryID, targetID);
        }
    }

}

// -----------------------------------------------------------------------
// 衝突判定の実装 (CheckAndResolve)
// -----------------------------------------------------------------------
void PhysicsSystem::CheckAndResolve(EntityID playerID, EntityID otherID) {
    auto registry = pWorld->GetRegistry();

    auto& pTrans = registry->GetComponent<TransformComponent>(playerID);
    auto& pCol = registry->GetComponent<ColliderComponent>(playerID);
    auto& pComp = registry->GetComponent<PlayerComponent>(playerID);

    OBB boxOBB = GetOBB(otherID);

    // OBBから行列をロード
    XMMATRIX boxWorld = XMLoadFloat4x4(&boxOBB.worldMatrix);
    XMVECTOR det;
    XMMATRIX boxInvWorld = XMMatrixInverse(&det, boxWorld);

    // プレイヤーのカプセル定義
    float pRadius = pCol.radius * pTrans.scale.x;
    float pHeight = pCol.height * pTrans.scale.y;
    // カプセル芯の計算
    float cylinderLen = std::max<float>(0.0f, pHeight - 2.0f * pRadius);
    float halfLen = cylinderLen * 0.5f;
    XMVECTOR pos = XMLoadFloat3(&pTrans.position);
    XMVECTOR up = XMVectorSet(0, 1, 0, 0);
    // 回転を考慮する場合
    XMVECTOR pStartW = pos - up * halfLen;
    XMVECTOR pEndW = pos + up * halfLen;

    float radius = pRadius;
    float maxPenetration = -1.0f;
    XMVECTOR finalPushW = XMVectorZero();
    bool isHit = false;

    // --- [判定A] カプセル線分 vs 箱 (ローカル空間) ---
    XMVECTOR pStartL = XMVector3TransformCoord(pStartW, boxInvWorld);
    XMVECTOR pEndL = XMVector3TransformCoord(pEndW, boxInvWorld);
    float hx = boxOBB.extents.x;
    float hy = boxOBB.extents.y;
    float hz = boxOBB.extents.z;

    XMVECTOR segmentVec = pEndL - pStartL;
    float segLen = XMVectorGetX(XMVector3Length(segmentVec));
    // 判定密度
    int steps = static_cast<int>(segLen / (radius * 0.05f)) + 2;

    for (int i = 0; i < steps; ++i) {
        float t = (float)i / (steps - 1);
        if (steps <= 1) t = 0.5f;
        XMVECTOR pointL = pStartL + segmentVec * t;
        XMFLOAT3 p; XMStoreFloat3(&p, pointL);

        float cx = std::max<float>(-hx, std::min<float>(p.x, hx));
        float cy = std::max<float>(-hy, std::min<float>(p.y, hy));
        float cz = std::max<float>(-hz, std::min<float>(p.z, hz));

        float dx = p.x - cx; float dy = p.y - cy; float dz = p.z - cz;
        float distSq = dx * dx + dy * dy + dz * dz;

        if (distSq < (radius * radius) + 0.0001f) {
            float dist = std::sqrt(distSq);
            float pen = radius - dist;
            if (pen <= 0.0f) pen = 0.001f;
            XMVECTOR pushL;
            if (dist > 0.00001f) {
                pushL = XMVectorSet(dx / dist, dy / dist, dz / dist, 0);
            }
            else {
                float dX = hx - std::abs(p.x); float dY = hy - std::abs(p.y); float dZ = hz - std::abs(p.z);
                if (dX < dY && dX < dZ) pushL = XMVectorSet((p.x > 0 ? 1.0f : -1.0f), 0, 0, 0);
                else if (dY < dZ)       pushL = XMVectorSet(0, (p.y > 0 ? 1.0f : -1.0f), 0, 0);
                else                    pushL = XMVectorSet(0, 0, (p.z > 0 ? 1.0f : -1.0f), 0);
                pen = radius + std::min({ dX, dY, dZ });
            }
            if (pen > maxPenetration) {
                maxPenetration = pen;
                finalPushW = XMVector3TransformNormal(pushL, boxWorld) * pen;
                isHit = true;
            }
        }
    }

    // --- [判定B & C] 箱の12本の辺 vs カプセルの芯 (ワールド空間) ---
    XMFLOAT3 c[8] = {
        {-hx,-hy,-hz}, { hx,-hy,-hz}, {-hx, hy,-hz}, { hx, hy,-hz},
        {-hx,-hy, hz}, { hx,-hy, hz}, {-hx, hy, hz}, { hx, hy, hz}
    };
    XMVECTOR v[8];
    for (int i = 0; i < 8; ++i) v[i] = XMVector3TransformCoord(XMLoadFloat3(&c[i]), boxWorld);

    int edges[12][2] = {
        {0,1}, {2,3}, {4,5}, {6,7}, // X
        {0,2}, {1,3}, {4,6}, {5,7}, // Y
        {0,4}, {1,5}, {2,6}, {3,7}  // Z
    };

    for (int i = 0; i < 12; ++i) {
        XMVECTOR edgeP1 = v[edges[i][0]];
        XMVECTOR edgeP2 = v[edges[i][1]];

        XMVECTOR ptOnCapsule, ptOnBoxEdge;
        float distSq = SegmentToSegmentDistSq(pStartW, pEndW, edgeP1, edgeP2, ptOnCapsule, ptOnBoxEdge);

        if (distSq < radius * radius) {
            float dist = std::sqrt(distSq);
            float pen = radius - dist;

            XMVECTOR pushDirW;
            if (dist > 0.00001f) {
                pushDirW = (ptOnCapsule - ptOnBoxEdge) / dist;
            }
            else {
                XMVECTOR center = XMVector3TransformCoord(XMVectorZero(), boxWorld);
                pushDirW = XMVector3Normalize(ptOnCapsule - center);
            }

            if (pen > maxPenetration) {
                maxPenetration = pen;
                finalPushW = pushDirW * pen;
                isHit = true;
            }
        }
    }

    // --- 衝突解決 ---
    if (isHit) {
        //衝突ログ
        DebugLog("Hit Detected! Player vs Entity(%d)", otherID);
        // ---------------------------------------------------------
        // ★追加: ダメージ処理と消滅処理
        // ---------------------------------------------------------

        // 相手が StatsComponent (HP) を持っているか確認
        if (registry->HasComponent<StatusComponent>(otherID)) {
            auto& enemyStatus = registry->GetComponent<StatusComponent>(otherID);

            if (enemyStatus.invincibleTimer <= 0.0f) {
                // プレイヤーが StatsComponent (攻撃力) を持っているか確認
                if (registry->HasComponent<StatusComponent>(playerID)) {
                    auto& playerStatus = registry->GetComponent<StatusComponent>(playerID);

                    // ダメージを与える
                    enemyStatus.TakeDamage(playerStatus.attackPower);
                    DebugLog("Enemy(%d) HP: %d / %d", otherID, enemyStatus.hp, enemyStatus.maxHp);

                    //無敵時間をリセット
                    enemyStatus.invincibleTimer = 0.5f;

                    // HPが0になったら消滅（デストロイ）
                    if (enemyStatus.IsDead()) {
                        DebugLog("Enemy(%d) is Dead!", otherID);
                        pWorld->DestroyEntity(otherID);

                        // 相手が消えたので、これ以上の物理押し出しは不要
                        return;
                    }
                }
            }
        }
        XMVECTOR currentPos = XMLoadFloat3(&pTrans.position);
        currentPos += finalPushW;
        XMStoreFloat3(&pTrans.position, currentPos);

        XMVECTOR v = XMLoadFloat3(&pComp.velocity);
        XMVECTOR pushDir = XMVector3Normalize(finalPushW);

        float dot = XMVectorGetX(XMVector3Dot(v, pushDir));
        if (dot < 0.0f) {
            v = v - pushDir * dot;
            XMStoreFloat3(&pComp.velocity, v);
        }

        if (XMVectorGetY(finalPushW) > 0.001f) {
            if (XMVectorGetY(pushDir) > 0.6f) {
                pComp.isGrounded = true;
            }
        }
        else if (XMVectorGetY(finalPushW) < -0.001f) {
            if (XMVectorGetY(pushDir) < -0.6f && pComp.velocity.y > 0) {
                pComp.velocity.y = 0;
            }
        }
    }
}
// -----------------------------------------------------------------------
// 攻撃判定の実装 (CheckAttackHit)
// -----------------------------------------------------------------------
void PhysicsSystem::CheckAttackHit(EntityID attackID, EntityID targetID){
    auto registry = pWorld->GetRegistry();

    //攻撃判定
    auto& aTrans = registry->GetComponent<TransformComponent>(attackID);

    float radius = 0.5f * aTrans.scale.x;
    XMVECTOR attackPos = XMLoadFloat3(&aTrans.position);

    //相手OBB
    OBB targetOBB = GetOBB(targetID);
    XMMATRIX targetWorld = XMLoadFloat4x4(&targetOBB.worldMatrix);
    XMVECTOR det;
    XMMATRIX targetInvWorld = XMMatrixInverse(&det, targetWorld);

    //判定ロジック
    XMVECTOR centerL = XMVector3TransformCoord(attackPos, targetInvWorld);
    XMFLOAT3 p; XMStoreFloat3(&p, centerL);

    float hx = targetOBB.extents.x;
    float hy = targetOBB.extents.y;
    float hz = targetOBB.extents.z;

    //箱の表面上の最近点
    float cx = std::max<float>(-hx, std::min<float>(p.x, hx));
    float cy = std::max<float>(-hy, std::min<float>(p.y, hy));
    float cz = std::max<float>(-hz, std::min<float>(p.z, hz));

    //距離の2乗
    float dx = p.x - cx;
    float dy = p.y - cy;
    float dz = p.z - cz;
    float distSq = dx * dx + dy * dy + dz * dz;

    //半径内ならヒット
    if (distSq < radius * radius) {
        auto& targetStatus = registry->GetComponent<StatusComponent>(targetID);

        //無敵時間チェック
        if (targetStatus.invincibleTimer <= 0.0f) {
            auto& attackBox = registry->GetComponent<AttackBoxComponent>(attackID);

            //ダメージ適用
            targetStatus.TakeDamage(attackBox.damage);
            DebugLog("Attack Hit! Target(%d) HP: %d", targetID, targetStatus.hp);

            //無敵時間ヒット
            targetStatus.invincibleTimer = 0.5f;

            //死亡判定
            if (targetStatus.IsDead()) {
                DebugLog("Target(%d) Defeated!", targetID);
                pWorld->DestroyEntity(targetID);
            }
        }
    }

}

// 回復判定の実装 (CheckRecoveryHit)
void PhysicsSystem::CheckRecoveryHit(EntityID recoveryID, EntityID targetID) {
    auto registry = pWorld->GetRegistry();

    // 回復判定（球とみなす）
    auto& rTrans = registry->GetComponent<TransformComponent>(recoveryID);
    float radius = 0.5f * rTrans.scale.x;
    XMVECTOR recPos = XMLoadFloat3(&rTrans.position);

    // 相手（OBB）
    OBB targetOBB = GetOBB(targetID);
    XMMATRIX targetWorld = XMLoadFloat4x4(&targetOBB.worldMatrix);
    XMVECTOR det;
    XMMATRIX targetInvWorld = XMMatrixInverse(&det, targetWorld);

    // 判定ロジック (点vs箱)
    XMVECTOR centerL = XMVector3TransformCoord(recPos, targetInvWorld);
    XMFLOAT3 p; XMStoreFloat3(&p, centerL);

    float hx = targetOBB.extents.x;
    float hy = targetOBB.extents.y;
    float hz = targetOBB.extents.z;

    float cx = std::max<float>(-hx, std::min<float>(p.x, hx));
    float cy = std::max<float>(-hy, std::min<float>(p.y, hy));
    float cz = std::max<float>(-hz, std::min<float>(p.z, hz));

    float dx = p.x - cx; float dy = p.y - cy; float dz = p.z - cz;
    float distSq = dx * dx + dy * dy + dz * dz;

    // ヒット！
    if (distSq < radius * radius) {
        auto& targetStats = registry->GetComponent<StatusComponent>(targetID);
        auto& recBox = registry->GetComponent<RecoveryBoxComponent>(recoveryID);

        // HPが減っている時だけ回復
        if (targetStats.hp < targetStats.maxHp) {
            targetStats.hp += recBox.healAmount;
            if (targetStats.hp > targetStats.maxHp) targetStats.hp = targetStats.maxHp;

            DebugLog("Healed! Target(%d) HP: %d / %d", targetID, targetStats.hp, targetStats.maxHp);

            // 回復したら判定を消す (1回使い切り)
            // ※範囲持続回復にしたい場合はここを消す
            pWorld->DestroyEntity(recoveryID);
        }
    }
}
