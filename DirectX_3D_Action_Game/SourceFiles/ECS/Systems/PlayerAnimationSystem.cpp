/*===================================================================
// ファイル: PlayerAnimationSystem.cpp
// 概要: プレイヤーのアニメーション制御 (方向傾き対応版)
=====================================================================*/
#include "ECS/Systems/PlayerAnimationSystem.h"
#include "ECS/World.h"
#include "ECS/Components/PlayerPartComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/ActionComponent.h"
#include "ECS/Components/StatusComponent.h"
#include "App/Game.h"
#include <cmath>

using namespace DirectX;

void PlayerAnimationSystem::Update(float dt) {
    timeAccumulator += dt;
    auto registry = pWorld->GetRegistry();

    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (!registry->HasComponent<PlayerPartComponent>(id)) continue;

        auto& part = registry->GetComponent<PlayerPartComponent>(id);

        // 親チェック
        if (!registry->HasComponent<TransformComponent>(part.parentID)) continue;

        auto& parentTrans = registry->GetComponent<TransformComponent>(part.parentID);
        auto& parentPlayer = registry->GetComponent<PlayerComponent>(part.parentID);
        auto& parentAction = registry->GetComponent<ActionComponent>(part.parentID);

        // --- 速度情報の取得 ---
        // プレイヤーの移動速度 (X, Z) を取得
        float velX = parentPlayer.velocity.x;
        float velZ = parentPlayer.velocity.z;
        float yaw = parentTrans.rotation.y;

        float fwdSpeed = velX * sinf(yaw) + velZ * cosf(yaw);
        float rightSpeed = velX * cosf(yaw) - velZ * sinf(yaw);
        // 移動しているかどうか
        bool isMoving = (velX * velX + velZ * velZ) > 0.1f;
        bool isAttacking = parentAction.isAttacking;

        // --- アニメーション計算 ---

        // 1. 基本の浮遊 (Idle)
        float floatY = sinf(timeAccumulator * 2.0f) * 0.1f;

        // 2. 移動時の傾き (Tilt)
        // 速度に合わせて体を傾ける (ドローンのような挙動)
        // 前進(Z+) -> 前に傾く(X回転+)
        // 後退(Z-) -> 後ろに傾く(X回転-)
        // 右移動(X+) -> 右に傾く(Z回転-)
        // 左移動(X-) -> 左に傾く(Z回転+)

        float maxTilt = 0.3f; // 最大傾斜角
        float tiltSens = 0.05f; // 感度

        // 速度成分を回転に変換
        float tiltX = fwdSpeed * tiltSens;
        float tiltZ = rightSpeed * tiltSens;

        // 制限をかける (Clamp)
        if (tiltX > maxTilt) tiltX = maxTilt;
        if (tiltX < -maxTilt) tiltX = -maxTilt;
        if (tiltZ > maxTilt) tiltZ = maxTilt;
        if (tiltZ < -maxTilt) tiltZ = -maxTilt;


        // 部位ごとの個別処理
        XMVECTOR offsetVec = XMLoadFloat3(&part.baseOffset);
        XMVECTOR extraRot = XMVectorZero();
        XMVECTOR extraOffset = XMVectorZero();

        switch (part.partType) {
        case PartType::Head:
            // 頭はあまり傾けず、視線を安定させる（逆補正）
            offsetVec = XMVectorAdd(offsetVec, XMVectorSet(0, floatY * 0.5f, 0, 0));
            // 体が傾いても頭は水平を保とうとする演出
            extraRot = XMVectorSet(tiltX * 0.2f, 0, tiltZ * 0.2f, 0);
            break;

        case PartType::Body:
            // 体は移動方向にガッツリ傾く
            offsetVec = XMVectorAdd(offsetVec, XMVectorSet(0, floatY, 0, 0));
            extraRot = XMVectorSet(tiltX, 0, tiltZ, 0);
            break;

        case PartType::ArmRight:
        case PartType::ArmLeft:
            // 腕は慣性で少し遅れるイメージ（浮遊感）
            offsetVec = XMVectorAdd(offsetVec, XMVectorSet(0, floatY * 1.5f, 0, 0));
            // 傾きも少し控えめに
            extraRot = XMVectorSet(tiltX * 0.5f, 0, tiltZ * 0.5f, 0);

            // 攻撃モーション (右腕突き出し)
            if (isAttacking && part.partType == PartType::ArmRight) {
                float t = 1.0f - (parentAction.cooldownTimer / parentAction.attackCooldown);
                float thrust = sinf(t * XM_PI) * 2.5f; // 突き出し距離アップ

                // 親の向き(RotationY)を考慮して前方に突き出す計算
                // ここでは簡易的にローカルZ軸(体の正面)へ加算
                // 本来は行列計算が必要ですが、簡易実装としてオフセットZに加算
                // ※パーツのローカル座標系での前方へ
                extraOffset = XMVectorSet(0, 0, thrust, 0);
            }
            break;
        }

        // --- 最終座標の適用 ---

        // 1. パーツのベース回転 + アニメーション回転
        float finalRotX = part.baseRotation.x + XMVectorGetX(extraRot);
        float finalRotY = parentTrans.rotation.y + part.baseRotation.y;
        float finalRotZ = part.baseRotation.z + XMVectorGetZ(extraRot);

        // 2. 位置の計算
        // 親のY軸回転だけを適用して、体の周りに配置
        XMMATRIX parentRotMat = XMMatrixRotationY(parentTrans.rotation.y);

        // 攻撃時の突き出しなどは、パーツの向きに合わせて回転させる必要がある
        // ここでは「親の向き」に合わせてオフセットを回転
        XMVECTOR finalOffset = XMVector3TransformCoord(offsetVec + extraOffset, parentRotMat);
        XMVECTOR finalPos = XMLoadFloat3(&parentTrans.position) + finalOffset;

        // 3. コンポーネント更新
        auto& partTrans = registry->GetComponent<TransformComponent>(id);
        XMStoreFloat3(&partTrans.position, finalPos);
        partTrans.rotation = { finalRotX, finalRotY, finalRotZ };
    }
}