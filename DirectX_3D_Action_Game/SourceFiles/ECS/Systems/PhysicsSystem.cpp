#define NOMINMAX 

#include "App/Main.h"
#include "ECS/Systems/PhysicsSystem.h"
#include "ECS/World.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/ColliderComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/EnemyComponent.h"
#include "ECS/Components/AttackBoxComponent.h"
#include "ECS/Components/RecoveryBoxComponent.h"
#include "ECS/Components/AttackSphereComponent.h"
#include "ECS/Components/RecoverySphereComponent.h"
#include "ECS/Components/RolesComponent.h"
#include "ECS/Components/StatusComponent.h"
#include "ECS/Components/PlayerPartComponent.h"
#include <vector>
#include <cmath>
#include <algorithm> // std::max, std::min
#include <DirectXMath.h>
#include <DirectXCollision.h>

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
    //Type_None ならサイズ0のOBBを返して終わる
    if (col.type == ColliderType::Type_None) {
        obb.extents = { 0,0,0 };
        return obb;
    }
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
    else if (col.type == ColliderType::Type_Sphere) {
        // ★追加: 球体の場合 (半径をXYZすべてのExtentに適用して立方体扱いとする、または球として扱う)
        // OBBとして近似する場合は、半径を各軸のハーフサイズとします
        float scaledRadius = col.radius * trans.scale.x; // 一律Xスケール依存とする
        obb.extents = { scaledRadius, scaledRadius, scaledRadius };
    }
    else {
        // カプセルの場合 (BoxでもSphereでもないならCapsuleとみなす)
        float scaledRadius = col.radius * trans.scale.x;
        float scaledHeight = col.height * trans.scale.y;
        obb.extents = { scaledRadius, scaledHeight * 0.5f, scaledRadius };
    }
    return obb;
}

// -----------------------------------------------------------------------
// ★追加: レイキャストによる地面判定
// -----------------------------------------------------------------------
// 戻り値: ヒットしたかどうか (true/false)
// outDist: ヒットした場合の距離
static bool RaycastGround(World* world, XMVECTOR origin, float maxDist, float& outDist) {
    auto registry = world->GetRegistry();
    float closestDist = maxDist;
    bool hitAny = false;

    XMVECTOR dirDown = XMVectorSet(0, -1, 0, 0); // 真下

    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        // 地面(Ground)とみなせるものだけ判定
        // ここではColliderがあり、かつPlayer/Enemy/AttackBox等でないものを地面とみなす
        // (厳密にやるなら TagComponent で "Ground" をつけるのがベストですが、簡易判定)
        if (!registry->HasComponent<ColliderComponent>(id)) continue;
        if (!registry->HasComponent<TransformComponent>(id)) continue;

        // 除外対象
        if (registry->HasComponent<PlayerComponent>(id)) continue;
        if (registry->HasComponent<EnemyComponent>(id)) continue;
        if (registry->HasComponent<AttackBoxComponent>(id)) continue;
        if (registry->HasComponent<RecoveryBoxComponent>(id)) continue;
        if (registry->HasComponent<AttackSphereComponent>(id)) continue;
        if (registry->HasComponent<RecoverySphereComponent>(id)) continue;
        if (registry->HasComponent<PlayerPartComponent>(id)) continue;

        // OBB取得 (PhysicsSystemクラスのメソッドをstaticヘルパー化するか、ここでも同様の計算を行う)
        // ここでは簡易的にOBB計算を再実装（またはPhysicsSystem::GetOBBをpublic staticにして呼ぶ）
        auto& trans = registry->GetComponent<TransformComponent>(id);
        auto& col = registry->GetComponent<ColliderComponent>(id);
        if (col.type == ColliderType::Type_None) continue;

        // DirectX::BoundingOrientedBox を作成
        BoundingOrientedBox obb;
        obb.Center = trans.position;
        obb.Extents = {
            col.size.x * trans.scale.x * 0.5f,
            col.size.y * trans.scale.y * 0.5f,
            col.size.z * trans.scale.z * 0.5f
        };
        // 回転 (Quaternion)
        XMVECTOR Q = XMQuaternionRotationRollPitchYaw(trans.rotation.x, trans.rotation.y, trans.rotation.z);
        XMStoreFloat4(&obb.Orientation, Q);

        // レイ判定
        float dist = 0.0f;
        if (obb.Intersects(origin, dirDown, dist)) {
            if (dist < closestDist) {
                closestDist = dist;
                hitAny = true;
            }
        }
    }

    outDist = closestDist;
    return hitAny;
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

    // ---------------------------------------------------------
    // プレイヤーの物理挙動 (レイキャスト接地 + 横方向の押し出し)
    // ---------------------------------------------------------
    for (EntityID playerID = 0; playerID < ECSConfig::MAX_ENTITIES; ++playerID) {
        if (!registry->HasComponent<PlayerComponent>(playerID)) continue;
        if (!registry->HasComponent<ColliderComponent>(playerID)) continue;

        auto& pComp = registry->GetComponent<PlayerComponent>(playerID);
        auto& pTrans = registry->GetComponent<TransformComponent>(playerID);

        // 1. レイキャストで地面を探す
        // コア(胴体中心)から足元までの理想の高さ
        // EntityFactoryで bodyBaseY=0 にしたので、足先は -1.2f くらいにある
        // 浮遊感を出したいので、地面からコアまでの高さを 2.0f (足の下に0.8f空く) くらいに設定
        float hoverHeight = 2.0f;
        float rayDist = 0.0f;

        XMVECTOR rayOrigin = XMLoadFloat3(&pTrans.position);

        // レイキャスト実行 (少し長めに探索: hoverHeight + 1.0f)
        bool hitGround = RaycastGround(pWorld, rayOrigin, hoverHeight + 1.0f, rayDist);

        if (hitGround && rayDist <= hoverHeight) {
            // 接地している！
            pComp.isGrounded = true;

            // 高さを補正 (スナップ)
            // 現在のY座標だと rayDist の位置に地面がある。
            // 地面の位置 = currentY - rayDist
            // 目標のY座標 = 地面の位置 + hoverHeight
            float groundY = pTrans.position.y - rayDist;
            float targetY = groundY + hoverHeight;

            // 補間で滑らかに移動 (バネのような挙動)
            // position.y = Lerp(position.y, targetY, 0.2f); // 簡易Lerp
            // または直接代入してピタッと止める
            pTrans.position.y = targetY;

            // 落下速度リセット (ジャンプ中はリセットしないなどの制御が必要だが、
            // ここでは「地面に近いなら接地」として処理)
            if (pComp.velocity.y < 0) {
                pComp.velocity.y = 0;
            }
        }
        else {
            // 空中
            pComp.isGrounded = false;
        }

        // 2. 横方向の衝突判定 (壁など)
        for (EntityID otherID = 0; otherID < ECSConfig::MAX_ENTITIES; ++otherID) {
            if (playerID == otherID) continue;
            if (!registry->HasComponent<ColliderComponent>(otherID)) continue;

            // 地面(Ground)とのOBB衝突判定は、Y軸をRaycastに任せたので無視したいが、
            // 「壁」もGround扱いだとすり抜けてしまう。
            // 簡易的に CheckAndResolve を呼ぶが、Y軸の押し出しをキャンセルするなどの工夫が必要。
            // 今回は既存の CheckAndResolve をそのまま使い、壁ずりを機能させる。
            // (ただし CheckAndResolve が上に押し上げてしまうとガタつく可能性あり)

            CheckAndResolve(playerID, otherID);
        }
    }

    //EntityID playerID = ECSConfig::INVALID_ID;
    //for (EntityID playerID = 0; playerID < ECSConfig::MAX_ENTITIES; ++playerID) {
    //    if (!registry->HasComponent<PlayerComponent>(playerID))continue;
    //    if (!registry->HasComponent<ColliderComponent>(playerID))continue;
    //    //接地フラグをリセット
    //    auto& pComp = registry->GetComponent<PlayerComponent>(playerID);
    //    pComp.isGrounded = false;

    //    for (EntityID otherID = 0; otherID < ECSConfig::MAX_ENTITIES; ++otherID) {
    //        if (playerID == otherID) continue;
    //        if (!registry->HasComponent<ColliderComponent>(otherID)) continue;
    //        CheckAndResolve(playerID, otherID);
    //    }
    //}

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
    // ★追加: 攻撃球の判定ループ
    for (EntityID attackID = 0; attackID < ECSConfig::MAX_ENTITIES; ++attackID) {
        if (!registry->HasComponent<AttackSphereComponent>(attackID)) continue;

        auto& sphere = registry->GetComponent<AttackSphereComponent>(attackID);
        EntityID ownerID = sphere.ownerID;

        for (EntityID targetID = 0; targetID < ECSConfig::MAX_ENTITIES; ++targetID) {
            if (attackID == targetID) continue;
            if (targetID == ownerID) continue;
            if (!registry->HasComponent<ColliderComponent>(targetID)) continue;
            if (!registry->HasComponent<StatusComponent>(targetID)) continue;

            // ★Sphere用の判定関数を呼ぶ
            CheckAttackSphereHit(attackID, targetID);
        }
    }

    // ★追加: 回復球の判定ループ
    for (EntityID recoveryID = 0; recoveryID < ECSConfig::MAX_ENTITIES; ++recoveryID) {
        if (!registry->HasComponent<RecoverySphereComponent>(recoveryID)) continue;

        auto& sphere = registry->GetComponent<RecoverySphereComponent>(recoveryID);
        EntityID ownerID = sphere.ownerID;

        for (EntityID targetID = 0; targetID < ECSConfig::MAX_ENTITIES; ++targetID) {
            if (recoveryID == targetID) continue;
            if (!registry->HasComponent<PlayerComponent>(targetID)) continue;
            if (!registry->HasComponent<StatusComponent>(targetID)) continue;
            if (!registry->HasComponent<ColliderComponent>(targetID)) continue;

            CheckRecoverySphereHit(recoveryID, targetID);
        }
    }

}

// -----------------------------------------------------------------------
// 衝突判定の実装 (CheckAndResolve)
// -----------------------------------------------------------------------
void PhysicsSystem::CheckAndResolve(EntityID playerID, EntityID otherID) {
    auto registry = pWorld->GetRegistry();

    // 相手が「攻撃ボックス」や「回復ボックス」なら、物理的な衝突処理（押し出し）は一切しない
    if (registry->HasComponent<AttackBoxComponent>(otherID)) return;
    if (registry->HasComponent<RecoveryBoxComponent>(otherID)) return;
    if (registry->HasComponent<AttackSphereComponent>(otherID)) return;
    if (registry->HasComponent<RecoverySphereComponent>(otherID)) return;

    auto& otherCol = registry->GetComponent<ColliderComponent>(otherID);
    if (otherCol.type == ColliderType::Type_None) return;

    // ★重要追加2: 相手が「自分のパーツ」なら無視する (自己衝突防止)
    if (registry->HasComponent<PlayerPartComponent>(otherID)) {
        auto& part = registry->GetComponent<PlayerPartComponent>(otherID);
        if (part.parentID == (int)playerID) {
            return; // 自分の体の一部なので衝突しない
        }
    }

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
        // ---------------------------------------------------------
        // ★追加: ダメージ処理と消滅処理
        // ---------------------------------------------------------
        // 相手がプレイヤーならダメージ処理をスキップ！ (ここを追加)
        bool isTargetPlayer = registry->HasComponent<PlayerComponent>(otherID);
        // 相手が StatusComponent (HP) を持っていて、かつプレイヤーではない場合のみダメージ
        if (!isTargetPlayer && registry->HasComponent<StatusComponent>(otherID)) {

            // 自分(プレイヤー)もステータスを持っているならダメージ計算
            if (registry->HasComponent<StatusComponent>(playerID)) {
                auto& playerStatus = registry->GetComponent<StatusComponent>(playerID);
                auto& enemyStatus = registry->GetComponent<StatusComponent>(otherID);

                // プレイヤーの無敵時間がなければ食らう
                if (playerStatus.invincibleTimer <= 0.0f) {

                    // 敵の攻撃力があればそれを使う。なければ固定値10
                    int damage = (enemyStatus.attackPower > 0) ? enemyStatus.attackPower : 10;

                    playerStatus.TakeDamage(damage);
                    DebugLog("OUCH! Player Hit by Enemy! HP: %d", playerStatus.hp);

                    // プレイヤーを少し無敵にする
                    playerStatus.invincibleTimer = 1.0f;

                    // ★修正: ノックバック処理 (斜め後ろへ強く弾く)
                    XMVECTOR enemyPos = XMLoadFloat3(&boxOBB.center);
                    XMVECTOR myPos = pos;

                    // 敵から自分へのベクトル (水平成分のみ正規化して後退方向とする)
                    XMVECTOR dir = myPos - enemyPos;
                    dir = XMVectorSetY(dir, 0.0f); // Y成分を消す
                    dir = XMVector3Normalize(dir);

                    // 斜め上へ弾く (後退15, 上昇10)
                    XMVECTOR knockbackVel = dir * 15.0f;
                    knockbackVel = XMVectorSetY(knockbackVel, 10.0f);

                    XMStoreFloat3(&pComp.velocity, knockbackVel);

                    // 接地フラグを強制解除 (空中に飛ばす)
                    pComp.isGrounded = false;
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

    if (registry->HasComponent<ColliderComponent>(targetID)) {
        if (registry->GetComponent<ColliderComponent>(targetID).type == ColliderType::Type_None) return;
    }

    // 攻撃の持ち主を取得
    auto& attackBox = registry->GetComponent<AttackBoxComponent>(attackID);
    EntityID ownerID = attackBox.ownerID;

    // 勢力チェック (フレンドリーファイア防止)
    bool isOwnerPlayer = registry->HasComponent<PlayerComponent>(ownerID);
    bool isTargetPlayer = registry->HasComponent<PlayerComponent>(targetID);

    // 「持ち主」と「ターゲット」が同じ勢力（両方プレイヤー、または両方敵）なら判定しない
    if (isOwnerPlayer == isTargetPlayer) return;

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

        if (targetStatus.invincibleTimer <= 0.0f) {
            targetStatus.TakeDamage(attackBox.damage);
            targetStatus.invincibleTimer = 0.5f;
            DebugLog("Hit! Target:%d Dmg:%d HP:%d", targetID, attackBox.damage, targetStatus.hp);
            // ★修正: ノックバック処理
            if (isTargetPlayer) {
                auto& pTrans = registry->GetComponent<TransformComponent>(targetID);
                auto& pComp = registry->GetComponent<PlayerComponent>(targetID);

                XMVECTOR enemyPos;
                if (registry->HasComponent<TransformComponent>(ownerID)) {
                    enemyPos = XMLoadFloat3(&registry->GetComponent<TransformComponent>(ownerID).position);
                }
                else {
                    enemyPos = attackPos;
                }
                XMVECTOR targetPos = XMLoadFloat3(&pTrans.position);

                // 敵 -> 自分 へのベクトル
                XMVECTOR dir = targetPos - enemyPos;
                dir = XMVectorSetY(dir, 0.0f);

                // ★重なっている場合: プレイヤーの背中側へ飛ばす (Rotationを使って計算)
                if (XMVectorGetX(XMVector3LengthSq(dir)) < 0.001f) {
                    // プレイヤーの向きを取得して、その逆方向(後ろ)を計算
                    XMMATRIX rotMat = XMMatrixRotationY(pTrans.rotation.y);
                    // (0,0,-1) はローカルの後ろ。これを回転させてワールドの後ろにする
                    dir = XMVector3TransformCoord(XMVectorSet(0, 0, -1.0f, 0), rotMat);
                }
                else {
                    dir = XMVector3Normalize(dir);
                }

                // 斜め上へ弾く (後退15, 上昇10)
                // ※この速度が次のPlayerSystemで消されないように、PlayerSystem側の修正が必須です
                XMVECTOR knockbackVel = dir * 10.0f;
                knockbackVel = XMVectorSetY(knockbackVel, 5.0f);

                XMStoreFloat3(&pComp.velocity, knockbackVel);
                pComp.isGrounded = false;
            }

            // 死亡判定
            if (targetStatus.IsDead()) {
                DebugLog("Target(%d) Defeated!", targetID);

                // ★修正: プレイヤーなら削除しない (Deadアニメーションのため)
                if (!isTargetPlayer) {
                    pWorld->DestroyEntity(targetID);
                }
            }
        }
    }

}

// 回復判定の実装 (CheckRecoveryHit)
void PhysicsSystem::CheckRecoveryHit(EntityID recoveryID, EntityID targetID) {
    auto registry = pWorld->GetRegistry();

    // ★追加: 相手が Type_None なら判定しない
    if (registry->HasComponent<ColliderComponent>(targetID)) {
        if (registry->GetComponent<ColliderComponent>(targetID).type == ColliderType::Type_None) return;
    }

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
void PhysicsSystem::CheckAttackSphereHit(EntityID attackID, EntityID targetID) {
    auto registry = pWorld->GetRegistry();

    // ★追加: 相手が Type_None なら判定しない
    if (registry->HasComponent<ColliderComponent>(targetID)) {
        if (registry->GetComponent<ColliderComponent>(targetID).type == ColliderType::Type_None) return;
    }

    auto& sphere = registry->GetComponent<AttackSphereComponent>(attackID);
    auto& trans = registry->GetComponent<TransformComponent>(attackID);
    EntityID ownerID = sphere.ownerID;
    // フレンドリーファイア防止
    bool isOwnerPlayer = registry->HasComponent<PlayerComponent>(sphere.ownerID);
    bool isTargetPlayer = registry->HasComponent<PlayerComponent>(targetID);
    if (isOwnerPlayer == isTargetPlayer) return;

    // 球とOBBの判定
    float radius = sphere.currentRadius; // 広がる半径を使用
    XMVECTOR spherePos = XMLoadFloat3(&trans.position);

    OBB targetOBB = GetOBB(targetID);
    XMMATRIX targetWorld = XMLoadFloat4x4(&targetOBB.worldMatrix);
    XMVECTOR det;
    XMMATRIX targetInvWorld = XMMatrixInverse(&det, targetWorld);

    XMVECTOR centerL = XMVector3TransformCoord(spherePos, targetInvWorld);
    XMFLOAT3 p; XMStoreFloat3(&p, centerL);

    float hx = targetOBB.extents.x;
    float hy = targetOBB.extents.y;
    float hz = targetOBB.extents.z;

    float cx = std::max<float>(-hx, std::min<float>(p.x, hx));
    float cy = std::max<float>(-hy, std::min<float>(p.y, hy));
    float cz = std::max<float>(-hz, std::min<float>(p.z, hz));

    float dx = p.x - cx; float dy = p.y - cy; float dz = p.z - cz;
    float distSq = dx * dx + dy * dy + dz * dz;

    if (distSq < radius * radius) {
        auto& targetStatus = registry->GetComponent<StatusComponent>(targetID);
        if (targetStatus.invincibleTimer <= 0.0f) {
            targetStatus.TakeDamage(sphere.damage);
            targetStatus.invincibleTimer = 0.5f;
            DebugLog("Sphere Hit! Target(%d)", targetID);

            // ★修正: ノックバック処理
            if (isTargetPlayer) {
                auto& pTrans = registry->GetComponent<TransformComponent>(targetID);
                auto& pComp = registry->GetComponent<PlayerComponent>(targetID);

                XMVECTOR enemyPos;
                if (registry->HasComponent<TransformComponent>(ownerID)) {
                    enemyPos = XMLoadFloat3(&registry->GetComponent<TransformComponent>(ownerID).position);
                }
                else {
                    enemyPos = XMLoadFloat3(&trans.position);
                }
                XMVECTOR targetPos = XMLoadFloat3(&pTrans.position);

                XMVECTOR dir = targetPos - enemyPos;
                dir = XMVectorSetY(dir, 0.0f);

                if (XMVectorGetX(XMVector3LengthSq(dir)) < 0.001f) {
                    XMMATRIX rotMat = XMMatrixRotationY(pTrans.rotation.y);
                    dir = XMVector3TransformCoord(XMVectorSet(0, 0, -1.0f, 0), rotMat);
                }
                else {
                    dir = XMVector3Normalize(dir);
                }

                XMVECTOR knockbackVel = dir * 15.0f;
                knockbackVel = XMVectorSetY(knockbackVel, 10.0f);
                XMStoreFloat3(&pComp.velocity, knockbackVel);
                pComp.isGrounded = false;
            }

            if (targetStatus.IsDead()) {
                // ★修正: プレイヤーなら削除しない
                if (!isTargetPlayer) {
                    pWorld->DestroyEntity(targetID);
                }
            }
        }
    }
}

void PhysicsSystem::CheckRecoverySphereHit(EntityID recoveryID, EntityID targetID) {
    auto registry = pWorld->GetRegistry();

    // ★追加: 相手が Type_None なら判定しない
    if (registry->HasComponent<ColliderComponent>(targetID)) {
        if (registry->GetComponent<ColliderComponent>(targetID).type == ColliderType::Type_None) return;
    }

    auto& sphere = registry->GetComponent<RecoverySphereComponent>(recoveryID);
    auto& trans = registry->GetComponent<TransformComponent>(recoveryID);

    float radius = sphere.currentRadius;
    XMVECTOR spherePos = XMLoadFloat3(&trans.position);

    OBB targetOBB = GetOBB(targetID);
    // ... (行列計算は上と同じ) ...
    XMMATRIX targetWorld = XMLoadFloat4x4(&targetOBB.worldMatrix);
    XMVECTOR det;
    XMMATRIX targetInvWorld = XMMatrixInverse(&det, targetWorld);
    XMVECTOR centerL = XMVector3TransformCoord(spherePos, targetInvWorld);
    XMFLOAT3 p; XMStoreFloat3(&p, centerL);
    float hx = targetOBB.extents.x; float hy = targetOBB.extents.y; float hz = targetOBB.extents.z;
    float cx = std::max<float>(-hx, std::min<float>(p.x, hx));
    float cy = std::max<float>(-hy, std::min<float>(p.y, hy));
    float cz = std::max<float>(-hz, std::min<float>(p.z, hz));
    float dx = p.x - cx; float dy = p.y - cy; float dz = p.z - cz;
    float distSq = dx * dx + dy * dy + dz * dz;

    if (distSq < radius * radius) {
        auto& targetStats = registry->GetComponent<StatusComponent>(targetID);
        if (targetStats.hp < targetStats.maxHp) {
            targetStats.hp += sphere.healAmount;
            if (targetStats.hp > targetStats.maxHp) targetStats.hp = targetStats.maxHp;
            DebugLog("Sphere Heal! Target(%d)", targetID);
            // 回復球は「使い切り」にしたい場合はここでDestroy
            // pWorld->DestroyEntity(recoveryID);
            // 「範囲持続回復」にしたい場合はDestroyしない
        }
    }
}
