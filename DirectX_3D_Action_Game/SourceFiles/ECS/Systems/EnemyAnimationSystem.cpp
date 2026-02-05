#include "ECS/Systems/EnemyAnimationSystem.h"
#include "ECS/World.h"
#include "ECS/Components/EnemyPartComponent.h" // エネミー用パーツ
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/EnemyComponent.h"
#include "ECS/Components/StatusComponent.h"
#include <cmath>
#include <algorithm>

using namespace DirectX;

// ピボット回転計算 (共通ヘルパー)
static XMVECTOR RotateAroundPivot(XMVECTOR point, XMVECTOR pivot, XMVECTOR rot) {
    XMVECTOR dir = point - pivot;
    XMMATRIX M = XMMatrixRotationRollPitchYawFromVector(rot);
    dir = XMVector3TransformCoord(dir, M);
    return pivot + dir;
}

void EnemyAnimationSystem::Update(float dt) {
    timeAccumulator += dt;
    auto registry = pWorld->GetRegistry();

    // 全エンティティから「エネミーのパーツ」を持っているものを探す
    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (!registry->HasComponent<EnemyPartComponent>(id)) continue;

        auto& part = registry->GetComponent<EnemyPartComponent>(id);

        // 親（本体）が存在するか確認
        if (!registry->HasComponent<TransformComponent>(part.parentID)) continue;

        auto& parentTrans = registry->GetComponent<TransformComponent>(part.parentID);
        // エネミー情報の取得（死んでいるかなどのチェック用）
        bool isDead = false;
        if (registry->HasComponent<StatusComponent>(part.parentID)) {
            if (registry->GetComponent<StatusComponent>(part.parentID).hp <= 0) isDead = true;
        }

        // --- アニメーションパラメータ ---
        XMVECTOR bodyOffset = XMVectorZero();
        XMVECTOR bodyRot = XMVectorZero();
        XMVECTOR localRot = XMVectorZero();

        // 1. 共通: 呼吸モーション (上下にふわふわ)
        if (!isDead) {
            float floatY = sinf(timeAccumulator * 3.0f + part.parentID) * 0.05f; // IDで位相をずらす
            bodyOffset = XMVectorAdd(bodyOffset, XMVectorSet(0, floatY, 0, 0));
        }

        // 2. 部位ごとの固有モーション (拡張版)
        if (part.partType == EnemyPartType::ArmLeft || part.partType == EnemyPartType::ArmRight) {
            // 腕をゆらゆらさせる
            float sway = sinf(timeAccumulator * 5.0f + part.parentID) * 0.1f;
            localRot = XMVectorSet(sway, 0, 0, 0);
        }
        else if (part.partType == EnemyPartType::LegLeft || part.partType == EnemyPartType::LegRight) {
            // 足を前後に（簡易歩行）
            float walk = sinf(timeAccumulator * 8.0f) * 0.2f;
            // 左右で逆位相
            if (part.partType == EnemyPartType::LegRight) walk *= -1.0f;
            localRot = XMVectorSet(walk, 0, 0, 0);
        }
        else if (part.partType == EnemyPartType::Wing) {
            // 羽をパタパタ（ボスの翼はゆっくり）
            float speed = (part.parentID % 2 == 0) ? 2.0f : 10.0f; // ボスかどうか簡易判定（本来はComponentで見るべきだが）
            float flap = sinf(timeAccumulator * speed) * 0.15f;
            localRot = XMVectorSet(flap, 0, 0, 0);
        }
        else if (part.partType == EnemyPartType::Ring) {
            float speed = 1.0f;         // 基本速度
            float t = timeAccumulator;

            int variant = id % 3; // 0, 1, 2

            if (variant == 0) {
                // Ring 1 (大): Y軸回転 (水平スピン)
                // 90度傾けないので、そのままY軸で回せば水平回転になります
                localRot = XMVectorSet(0, t * speed * 0.5f, 0, 0);
            }
            else if (variant == 1) {
                // Ring 2 (中): X軸回転 (縦回転)
                // X軸で回すと、前転/後転のような動きになります
                localRot = XMVectorSet(t * speed * 0.8f, 0, 0, 0);
            }
            else {
                // Ring 3 (小): Z軸回転 (側転)
                // Z軸で回すと、正面から見て時計回りに回ります
                localRot = XMVectorSet(0, 0, t * speed * 1.2f, 0);
            }
        }
        else if (part.partType == EnemyPartType::Shield) {
            // ビット（シールド）を周囲に公転させる
            // ※BaseOffset自体を書き換えるのは少し複雑なので、ここでは自転のみ
            float spin = timeAccumulator * 3.0f;
            localRot = XMVectorSet(0, spin, 0, 0);
        }
        else if (part.partType == EnemyPartType::Thruster) {
            // スラスターの振動
            float vib = sinf(timeAccumulator * 30.0f) * 0.05f;
            bodyOffset = XMVectorAdd(bodyOffset, XMVectorSet(0, 0, vib, 0));
        }

        // 3. 死亡時の崩壊エフェクト (バラバラになる)
        if (isDead) {
            // 爆発するように広がる（簡易）
            // ※本来はPhysicsで飛ばすのが良いですが、ここでは見た目だけ
            float explode = 0.1f;
            // パーツごとの固有ベクトルへ飛ばす（簡易的にオフセット方向へ）
            XMVECTOR dir = XMLoadFloat3(&part.baseOffset);
            dir = XMVector3Normalize(dir);
            bodyOffset = XMVectorAdd(bodyOffset, dir * explode);
            // 落下
            bodyOffset = XMVectorAdd(bodyOffset, XMVectorSet(0, -0.5f, 0, 0));
        }

        // =========================================================
        // 最終適用 (親の行列に合わせて追従)
        // =========================================================

        // 1. 基準位置
        XMVECTOR currentPos = XMLoadFloat3(&part.baseOffset);
        XMVECTOR currentRot = XMVectorZero();

        // 2. ローカル回転・移動適用
        currentRot = XMVectorAdd(currentRot, localRot);
        currentPos = XMVectorAdd(currentPos, bodyOffset);

        // 3. 親のワールド変換 (回転 -> 平行移動)
        // エネミーのY軸回転
        XMMATRIX parentRotMat = XMMatrixRotationY(parentTrans.rotation.y);

        // 位置を回す
        XMVECTOR finalPosVec = XMVector3TransformCoord(currentPos, parentRotMat);
        // 親の位置を足す
        finalPosVec = XMVectorAdd(finalPosVec, XMLoadFloat3(&parentTrans.position));

        // 回転の合成
        float finalRotX = part.baseRotation.x + XMVectorGetX(currentRot);
        float finalRotY = parentTrans.rotation.y + part.baseRotation.y + XMVectorGetY(currentRot);
        float finalRotZ = part.baseRotation.z + XMVectorGetZ(currentRot);

        // 4. TransformComponentに書き込み
        if (registry->HasComponent<TransformComponent>(id)) {
            auto& partTrans = registry->GetComponent<TransformComponent>(id);
            XMStoreFloat3(&partTrans.position, finalPosVec);
            partTrans.rotation = { finalRotX, finalRotY, finalRotZ };
            // スケールは生成時のまま維持
        }
    }
}