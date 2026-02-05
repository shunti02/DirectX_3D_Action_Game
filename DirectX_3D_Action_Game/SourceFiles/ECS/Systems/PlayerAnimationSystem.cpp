/*===================================================================
// ファイル: PlayerAnimationSystem.cpp
// 概要: プレイヤーのアニメーション制御 (ジャンプ優先・完全版)
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
#include <algorithm>

using namespace DirectX;

// ---------------------------------------------------------
// ヘルパー関数
// ---------------------------------------------------------

// 線形補間
static float Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

// ピボット回転計算 (点を中心に回転させる)
static XMVECTOR RotateAroundPivot(XMVECTOR point, XMVECTOR pivot, XMVECTOR rot) {
    // 1. 原点へ移動 (ピボットを原点に)
    XMVECTOR dir = point - pivot;

    // 2. 回転行列作成 (Roll=Z, Pitch=X, Yaw=Y)
    XMMATRIX M = XMMatrixRotationRollPitchYawFromVector(rot);

    // 3. 回転適用
    dir = XMVector3TransformCoord(dir, M);

    // 4. 元の位置へ戻す
    return pivot + dir;
}

// ---------------------------------------------------------
// メイン更新処理
// ---------------------------------------------------------
void PlayerAnimationSystem::Update(float dt) {
    timeAccumulator += dt;
    auto registry = pWorld->GetRegistry();

    // ★ピボット位置 (肩の高さY=2.4fに合わせて設定)
    XMVECTOR pivotShoulderR = XMVectorSet(0.7f, 0.7f, 0.0f, 0.0f);
    XMVECTOR pivotShoulderL = XMVectorSet(-0.7f, 0.7f, 0.0f, 0.0f);
    // 脚の付け根
    XMVECTOR pivotLegR = XMVectorSet(0.4f, -0.3f, 0.0f, 0.0f);
    XMVECTOR pivotLegL = XMVectorSet(-0.4f, -0.3f, 0.0f, 0.0f);

    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (!registry->HasComponent<PlayerPartComponent>(id)) continue;

        auto& part = registry->GetComponent<PlayerPartComponent>(id);
        if (!registry->HasComponent<TransformComponent>(part.parentID)) continue;

        auto& parentTrans = registry->GetComponent<TransformComponent>(part.parentID);
        auto& parentPlayer = registry->GetComponent<PlayerComponent>(part.parentID);
        auto& parentAction = registry->GetComponent<ActionComponent>(part.parentID);
        auto& parentStatus = registry->GetComponent<StatusComponent>(part.parentID);

        // --- 状態判定 ---
        bool isDead = parentStatus.IsDead();
        bool isHurt = !isDead && (parentStatus.invincibleTimer > 0.0f);

        // ジャンプ中: 接地していない
        bool isJumping = !isDead && !isHurt && !parentPlayer.isGrounded;

        // 攻撃中: フラグが立っている
        bool isAttacking = !isDead && !isHurt && parentAction.isAttacking;

        float speedSq = parentPlayer.velocity.x * parentPlayer.velocity.x + parentPlayer.velocity.z * parentPlayer.velocity.z;
        bool isMoving = !isDead && !isHurt && (speedSq > 0.1f);

        // =========================================================
        // アニメーションパラメータ計算
        // =========================================================

        // 1. 全身共通 (Body Transform)
        XMVECTOR bodyOffset = XMVectorZero(); // 体全体の位置ズレ
        XMVECTOR bodyRot = XMVectorZero();    // 体全体の回転

        // 2. パーツ個別 (Local Transform)
        XMVECTOR localRot = XMVectorZero();   // パーツ自体の回転
        XMVECTOR pivotPos = XMVectorZero();   // 回転の中心点
        bool usePivot = false;

        // パーツごとにピボットを設定
        if (part.partType == PartType::ShoulderRight || part.partType == PartType::ArmRight || part.partType == PartType::HandRight) {
            pivotPos = pivotShoulderR; usePivot = true;
        }
        else if (part.partType == PartType::ShoulderLeft || part.partType == PartType::ArmLeft || part.partType == PartType::HandLeft) {
            pivotPos = pivotShoulderL; usePivot = true;
        }
        else if (part.partType == PartType::LegRight) {
            pivotPos = pivotLegR; usePivot = true;
        }
        else if (part.partType == PartType::LegLeft) {
            pivotPos = pivotLegL; usePivot = true;
        }

        // --- A. 共通: ぷかぷか (Idle) ---
        if (!isDead) {
            float floatY = sinf(timeAccumulator * 2.0f) * 0.1f;
            bodyOffset = XMVectorAdd(bodyOffset, XMVectorSet(0, floatY, 0, 0));

            if (!isAttacking && !isJumping && !isHurt) {

                // タイプ取得
                PlayerType pType = parentPlayer.type;

                // 基本角度 (Type A用)
                float armAngleR = -1.0f; // 右腕 (Z回転)
                float armAngleL = 1.0f; // 左腕 (Z回転)

                // Type B (肩幅が広いので、少し開き気味にする)
                if (pType == PlayerType::BusterGuard) {
                    armAngleR = -0.1f; // あまり閉じない
                    armAngleL = 0.1f;
                }
                // Type C (細身なので、少し狭めるがクロスしない程度)
                else if (pType == PlayerType::PlasmaSniper) {
                    armAngleR = -0.1f;
                    armAngleL = 0.1f;
                }

                // 右腕
                if (part.partType == PartType::ShoulderRight || part.partType == PartType::ArmRight || part.partType == PartType::HandRight) {
                    usePivot = true; pivotPos = pivotShoulderR;
                    float sway = sinf(timeAccumulator * 3.0f) * 0.05f;
                    localRot = XMVectorSet(0, 0, armAngleR + sway, 0);
                }

                // 左腕
                if (part.partType == PartType::ShoulderLeft || part.partType == PartType::ArmLeft || part.partType == PartType::HandLeft) {
                    usePivot = true; pivotPos = pivotShoulderL;
                    float sway = sinf(timeAccumulator * 3.0f + XM_PI) * 0.05f;
                    localRot = XMVectorSet(0, 0, armAngleL + sway, 0);
                }
            }
        }

        // --- B. 移動: 進行方向への傾き (Move) ---
        if (isMoving || isJumping) {
            float yaw = parentTrans.rotation.y;
            float velX = parentPlayer.velocity.x;
            float velZ = parentPlayer.velocity.z;
            // ローカル座標系での速度成分
            float fwd = velX * sinf(yaw) + velZ * cosf(yaw);
            float right = velX * cosf(yaw) - velZ * sinf(yaw);

            float tx = std::clamp(fwd * 0.05f, -0.4f, 0.4f);
            float tz = std::clamp(right * 0.05f, -0.4f, 0.4f);

            bodyRot = XMVectorAdd(bodyRot, XMVectorSet(tx, 0, tz, 0));
        }

        // --- C. 攻撃 ---
        // ★修正: ジャンプ中は攻撃アニメーションを適用しない (!isJumping)
        if (isAttacking && !isJumping) {
            float t = parentAction.cooldownTimer / parentAction.attackCooldown; // 1.0 -> 0.0
            // タイプ取得
            PlayerType pType = parentPlayer.type;

            // =================================================
            // Type A: 袈裟斬り (既存)
            // =================================================
            if (pType == PlayerType::AssaultStriker) {
            // 1. 全身の動き (Body)
            float twistY = 0.0f; float leanX = 0.0f; float stepZ = 0.0f; float stepY = 0.0f;

            if (t > 0.4f) {
                // [溜め]
                // 体を「右」に捻る (右肩を引く)
                float subT = (1.0f - t) / 0.6f;
                float ease = sinf(subT * XM_PIDIV2);

                // ★修正: プラス方向(右回転)で右肩を引く
                twistY = Lerp(0.0f, -0.8f, ease);
                leanX = Lerp(0.0f, 0.5f, ease);  // 前傾
                stepZ = Lerp(0.0f, -0.3f, ease); // 引く
                stepY = Lerp(0.0f, -0.4f, ease); // 沈む
            }
            else if (t > 0.15f) {
                // [抜刀]
                // 体を「左」に捻り戻す (右肩を前へ)
                float subT = (0.4f - t) / 0.25f;
                float ease = 1.0f - powf(2.0f, -10.0f * subT);

                twistY = Lerp(0.8f, -1.3f, ease); // ★左へ回転
                leanX = Lerp(0.5f, 0.8f, ease);  // 踏み込む
                stepZ = Lerp(-0.3f, 1.5f, ease); // 前へ
                stepY = Lerp(-0.4f, -0.5f, ease);// 低く
            }
            else {
                // [残心]
                float subT = (0.15f - t) / 0.15f;
                twistY = Lerp(-1.0f, 0.0f, subT);
                leanX = Lerp(0.8f, 0.0f, subT);
                stepZ = Lerp(1.5f, 0.0f, subT);
                stepY = Lerp(-0.5f, 0.0f, subT);
            }
            bodyOffset = XMVectorAdd(bodyOffset, XMVectorSet(0, stepY, stepZ, 0));
            bodyRot = XMVectorAdd(bodyRot, XMVectorSet(leanX, twistY, 0, 0));

            // 2. 右腕全体 (Shoulder/Arm/Hand)
            if (part.partType == PartType::ShoulderRight || part.partType == PartType::ArmRight || part.partType == PartType::HandRight) {
                float rX = 0, rY = 0, rZ = 0;

                if (t > 0.4f) {
                    // [溜め] 
                    // 右手は「左腰」へ行く必要があります。
                    // 以前の値では右後ろに行っていた可能性が高いので、符号を逆にします。
                    float subT = (1.0f - t) / 0.6f;
                    float ease = sinf(subT * XM_PIDIV2);

                    // Y回転: マイナスで左方向(内側)へ
                    rY = Lerp(0.0f, -2.0f, ease);   // ★左腰へクロス
                    rZ = Lerp(0.0f, -0.5f, ease);   // 少し下げる
                    rX = Lerp(0.0f, -0.5f, ease);   // 軽く捻る
                }
                else if (t > 0.15f) {
                    // [一閃]
                    // 右手は「右前方」へ
                    float subT = (0.4f - t) / 0.25f;
                    float ease = 1.0f - powf(2.0f, -10.0f * subT);

                    // Y回転: プラスで右方向(外側)へ
                    rY = Lerp(-2.0f, 1.5f, ease);   // ★右へ振り抜く
                    rZ = Lerp(-0.5f, 0.0f, ease);   // 水平
                    rX = Lerp(-0.5f, 0.0f, ease);   // 戻す
                }
                else {
                    // [残心]
                    float subT = (0.15f - t) / 0.15f;
                    rY = Lerp(1.5f, 0.0f, subT);
                    rZ = Lerp(0.0f, 0.0f, subT);
                    rX = 0.0f;
                }
                localRot = XMVectorSet(rX, rY, rZ, 0);
            }

            // 3. 左足 (踏み込み)
            if (part.partType == PartType::LegLeft) {
                // 左足: 溜めで縮み、斬撃で踏み込む
                if (t > 0.4f) localRot = XMVectorAdd(localRot, XMVectorSet(0.8f, 0, 0, 0)); // 縮む
                else if (t > 0.15f) localRot = XMVectorAdd(localRot, XMVectorSet(-2.0f, 0, 0, 0)); // 踏み込む
                else {
                    float subT = (0.15f - t) / 0.15f;
                    float ang = Lerp(-1.0f, 0.0f, subT);
                    localRot = XMVectorAdd(localRot, XMVectorSet(ang, 0, 0, 0));
                }
            }

            // 4. 右足 (軸足)
            if (part.partType == PartType::LegRight) {
                if (t > 0.4f) localRot = XMVectorAdd(localRot, XMVectorSet(1.0f, 0, 0, 0)); // 縮む
                else if (t > 0.15f) localRot = XMVectorAdd(localRot, XMVectorSet(0.3f, 0, 0, 0)); // 残す
                else {
                    float subT = (0.15f - t) / 0.15f;
                    float ang = Lerp(0.3f, 0.0f, subT);
                    localRot = XMVectorAdd(localRot, XMVectorSet(ang, 0, 0, 0));
                }
            }

            // 5. 左腕 (鞘引き - 左後ろへ)
            if (part.partType == PartType::ShoulderLeft || part.partType == PartType::ArmLeft) {
                usePivot = true; pivotPos = pivotShoulderL;
                if (t <= 0.4f && t > 0.15f) localRot = XMVectorSet(0, -0.8f, -0.9f, 0);
            }
        }
        // =================================================
            // Type B: ハンマー・スマッシュ (パワフルな縦振り)
            // =================================================
            else if (pType == PlayerType::BusterGuard) {
                // 全身: 大きくのけぞって叩きつける
                float leanX = 0.0f; float jumpY = 0.0f;

                if (t > 0.5f) {
                    // [溜め] 上体を反らす(-X回転) + 少し浮く
                    float subT = (1.0f - t) / 0.5f;
                    leanX = Lerp(0.0f, -0.5f, subT);
                    jumpY = Lerp(0.0f, 0.5f, subT);
                }
                else if (t > 0.3f) {
                    // [叩きつけ] 急激に前傾(+X回転) + 着地
                    float subT = (0.5f - t) / 0.2f;
                    leanX = Lerp(-0.5f, 0.8f, subT);
                    jumpY = Lerp(0.5f, -0.2f, subT);
                }
                else {
                    // [硬直] 戻る
                    float subT = (0.3f - t) / 0.3f;
                    leanX = Lerp(0.8f, 0.0f, subT);
                    jumpY = Lerp(-0.2f, 0.0f, subT);
                }
                bodyRot = XMVectorAdd(bodyRot, XMVectorSet(leanX, 0, 0, 0));
                bodyOffset = XMVectorAdd(bodyOffset, XMVectorSet(0, jumpY, 0, 0));

                // 両腕: 万歳して振り下ろす
                // 腕パーツすべてに適用
                if (part.partType == PartType::ArmRight || part.partType == PartType::ArmLeft ||
                    part.partType == PartType::ShoulderRight || part.partType == PartType::ShoulderLeft ||
                    part.partType == PartType::HandRight || part.partType == PartType::HandLeft) {

                    float rX = 0;
                    if (t > 0.5f) {
                        // 万歳 (後ろへ)
                        float subT = (1.0f - t) / 0.5f;
                        rX = Lerp(0.0f, -2.5f, subT);
                    }
                    else if (t > 0.3f) {
                        // 振り下ろし (前へ)
                        float subT = (0.5f - t) / 0.2f;
                        rX = Lerp(-2.5f, 1.0f, subT);
                    }
                    else {
                        // 戻る
                        float subT = (0.3f - t) / 0.3f;
                        rX = Lerp(1.0f, 0.0f, subT);
                    }

                    // 個別回転に追加
                    localRot = XMVectorSet(rX, 0, 0, 0);
                }
            }
            // =================================================
            // Type C: プラズマ・スナイプ (射撃)
            // =================================================
            else if (pType == PlayerType::PlasmaSniper) {
                // 全身: 構え -> 反動で後退 -> 戻る
                float slideZ = 0.0f; float twistY = 0.0f;

                if (t > 0.8f) {
                    // [構え] 半身になる
                    float subT = (1.0f - t) / 0.2f;
                    twistY = Lerp(0.0f, -0.5f, subT);
                }
                else if (t > 0.6f) {
                    // [発射] 反動で後ろへスライド + 上体反らし
                    float subT = (0.8f - t) / 0.2f;
                    twistY = -0.5f;
                    slideZ = Lerp(0.0f, -0.8f, subT); // 後ろへスライド
                    bodyRot = XMVectorAdd(bodyRot, XMVectorSet(-0.2f * subT, 0, 0, 0)); // 上体反らし(Recoil)
                }
                else {
                    // [残心] ゆっくり戻る
                    float subT = (0.6f - t) / 0.6f;
                    twistY = Lerp(-0.5f, 0.0f, subT);
                    slideZ = Lerp(-0.8f, 0.0f, subT);
                }
                bodyOffset = XMVectorAdd(bodyOffset, XMVectorSet(0, 0, slideZ, 0));
                bodyRot = XMVectorAdd(bodyRot, XMVectorSet(0, twistY, 0, 0));

                // 右腕: 前に突き出す (銃を構える)
                if (part.partType == PartType::ArmRight || part.partType == PartType::ShoulderRight) {
                    float rX = 0.0f;
                    if (t > 0.8f) {
                        float subT = (1.0f - t) / 0.2f;
                        rX = Lerp(0.0f, -1.5f, subT); // 水平に上げる (90度)
                    }
                    else if (t > 0.6f) {
                        // 発射瞬間に跳ね上がる
                        float subT = (0.8f - t) / 0.2f;
                        rX = Lerp(-1.5f, -1.8f, subT);
                    }
                    else {
                        // 戻る
                        float subT = (0.6f - t) / 0.6f;
                        rX = Lerp(-1.8f, 0.0f, subT);
                    }
                    localRot = XMVectorSet(rX, 0, 0, 0);
                }
                // 左腕: 添えるだけ
                if (part.partType == PartType::ArmLeft || part.partType == PartType::ShoulderLeft) {
                    float rX = 0.0f; float rY = 0.0f;
                    if (t > 0.6f) {
                        rX = -0.5f; rY = 0.5f; // 胸元へ
                    }
                    else {
                        float subT = (0.6f - t) / 0.6f;
                        rX = Lerp(-0.5f, 0.0f, subT);
                        rY = Lerp(0.5f, 0.0f, subT);
                    }
                    localRot = XMVectorSet(rX, rY, 0, 0);
                }
            }
        }
        // --- D. ジャンプ: 万歳 (Jump) ---
        else if (isJumping) {
            // 体を伸ばす
            bodyOffset = XMVectorAdd(bodyOffset, XMVectorSet(0, 0.2f, 0, 0));

            // 左腕 (上へ)
            if (part.partType == PartType::ShoulderLeft || part.partType == PartType::ArmLeft || part.partType == PartType::HandLeft) {
                usePivot = true; pivotPos = pivotShoulderL;
                // 左腕(-X)に対し、+Z回転で上へ
                localRot = XMVectorSet(0, 0, -1.7f, 0);
            }
            // 右腕 (上へ)
            if (part.partType == PartType::ShoulderRight || part.partType == PartType::ArmRight || part.partType == PartType::HandRight) {
                usePivot = true; pivotPos = pivotShoulderR;
                // 右腕(+X)に対し、-Z回転で上へ
                localRot = XMVectorSet(0, 0, 1.7f, 0);
            }
            // 足を曲げる
            if (part.partType == PartType::LegLeft) {
                usePivot = true; pivotPos = pivotLegL;
                localRot = XMVectorSet(-0.5f, 0, 0, 0); // 後ろへ
            }
            if (part.partType == PartType::LegRight) {
                usePivot = true; pivotPos = pivotLegR;
                localRot = XMVectorSet(-0.8f, 0, 0, 0); // 右足は深く曲げる
            }
        }
        // --- E. 死亡: 膝をつく (Dead) ---
        else if (isDead) {
            bodyOffset = XMVectorAdd(bodyOffset, XMVectorSet(0, -0.8f, 0, 0));
            bodyRot = XMVectorAdd(bodyRot, XMVectorSet(0.5f, 0, 0, 0));

            if (part.partType == PartType::Head) {
                localRot = XMVectorAdd(localRot, XMVectorSet(0.8f, 0, 0, 0));
                bodyOffset = XMVectorAdd(bodyOffset, XMVectorSet(0, -0.1f, 0.2f, 0));
            }
            if (part.partType == PartType::ShoulderRight || part.partType == PartType::ArmRight) {
                usePivot = true; pivotPos = pivotShoulderR;
                localRot = XMVectorSet(0, 0, 1.2f, 0); // 下(-Z)
            }
            if (part.partType == PartType::ShoulderLeft || part.partType == PartType::ArmLeft) {
                usePivot = true; pivotPos = pivotShoulderL;
                localRot = XMVectorSet(0, 0, -1.2f, 0); // 下(+Z)
            }
            if (part.partType == PartType::LegLeft) {
                usePivot = true; pivotPos = pivotLegL;
                localRot = XMVectorSet(-1.5f, 0, 0, 0); // 90度
            }
            if (part.partType == PartType::LegRight) {
                usePivot = true; pivotPos = pivotLegR;
                localRot = XMVectorSet(-1.5f, 0, 0, 0); // 90度
            }
        }
        // --- F. 被弾: ノックバック (Hit) ---
        else if (isHurt) {
            float s = sinf(timeAccumulator * 60.0f) * 0.05f;
            bodyOffset = XMVectorAdd(bodyOffset, XMVectorSet(s, s, 0, 0));
            bodyRot = XMVectorAdd(bodyRot, XMVectorSet(-0.5f, 0, 0, 0)); // 仰け反り

            if (part.partType == PartType::ArmRight) {
                usePivot = true; pivotPos = pivotShoulderR;
                localRot = XMVectorSet(0, 0, -1.0f, 0);
            }
            if (part.partType == PartType::ArmLeft) {
                usePivot = true; pivotPos = pivotShoulderL;
                localRot = XMVectorSet(0, 0, 1.0f, 0);
            }
        }

        // =========================================================
        // 最終適用
        // =========================================================

        XMVECTOR currentPos = XMLoadFloat3(&part.baseOffset);
        XMVECTOR currentRot = XMVectorZero();

        // 1. パーツ個別 (ピボット)
        if (usePivot) {
            currentPos = RotateAroundPivot(currentPos, pivotPos, localRot);
            currentRot = XMVectorAdd(currentRot, localRot);
        }
        else {
            currentRot = XMVectorAdd(currentRot, localRot);
        }

        // 2. 全身連動
        currentPos = RotateAroundPivot(currentPos, XMVectorZero(), bodyRot);
        currentRot = XMVectorAdd(currentRot, bodyRot);
        currentPos = XMVectorAdd(currentPos, bodyOffset);

        // 3. ワールド変換
        XMMATRIX parentRotMat = XMMatrixRotationY(parentTrans.rotation.y);
        XMVECTOR finalPosVec = XMVector3TransformCoord(currentPos, parentRotMat);
        finalPosVec = XMVectorAdd(finalPosVec, XMLoadFloat3(&parentTrans.position));

        float finalRotX = part.baseRotation.x + XMVectorGetX(currentRot);
        float finalRotY = parentTrans.rotation.y + part.baseRotation.y + XMVectorGetY(currentRot);
        float finalRotZ = part.baseRotation.z + XMVectorGetZ(currentRot);

        // 4. 更新
        auto& partTrans = registry->GetComponent<TransformComponent>(id);
        DirectX::XMStoreFloat3(&partTrans.position, finalPosVec);
        partTrans.rotation = { finalRotX, finalRotY, finalRotZ };
    }
}