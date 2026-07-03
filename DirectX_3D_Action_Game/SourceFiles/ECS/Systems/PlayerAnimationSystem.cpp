#include "ECS/Systems/PlayerAnimationSystem.h"
#include "ECS/World.h"
#include "ECS/Components/PlayerPartComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/ActionComponent.h"
#include "ECS/Components/StatusComponent.h"
#include "Engine/AnimationManager.h"
#include "App/Game.h"
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <functional>

using namespace DirectX;

static float Lerp(float a, float b, float t) { return a + (b - a) * t; }

static XMFLOAT3 ExtractEulerAngles(const XMMATRIX& mat) {
    XMFLOAT4X4 m; XMStoreFloat4x4(&m, mat);
    float pitch = asinf(-std::clamp(m._32, -1.0f, 1.0f));
    float yaw = 0.0f, roll = 0.0f;
    if (cosf(pitch) > 0.0001f) { yaw = atan2f(m._31, m._33); roll = atan2f(m._12, m._22); }
    else { yaw = atan2f(-m._13, m._11); roll = 0.0f; }
    return { pitch, yaw, roll };
}

struct AnimState {
    XMVECTOR waistOffset = XMVectorZero();
    XMVECTOR waistRot = XMVectorZero();
    XMVECTOR bodyRot = XMVectorZero();
    XMVECTOR headRot = XMVectorZero();
    XMVECTOR rightShoulderRot = XMVectorZero();
    XMVECTOR rightArmRot = XMVectorZero();
    XMVECTOR leftShoulderRot = XMVectorZero();
    XMVECTOR leftArmRot = XMVectorZero();
    XMVECTOR rightLegRot = XMVectorZero();
    XMVECTOR leftLegRot = XMVectorZero();
};

void PlayerAnimationSystem::Update(float dt) {
    timeAccumulator += dt;
    auto registry = pWorld->GetRegistry();

    std::unordered_map<EntityID, AnimState> animStates;
    std::unordered_map<EntityID, EntityID> partToCoreMap;

    for (EntityID coreID = 0; coreID < ECSConfig::MAX_ENTITIES; ++coreID) {
        if (!registry->HasComponent<PlayerComponent>(coreID) || !registry->HasComponent<TransformComponent>(coreID)) continue;

        auto& pComp = registry->GetComponent<PlayerComponent>(coreID);
        AnimState state;

        bool isDead = false;
        bool isHurt = false;
        if (registry->HasComponent<StatusComponent>(coreID)) {
            auto& status = registry->GetComponent<StatusComponent>(coreID);
            isDead = status.hp <= 0;
            isHurt = !isDead && (status.invincibleTimer > 0.0f);
        }

        bool isJumping = !isDead && !isHurt && !pComp.isGrounded;
        float speedSq = pComp.velocity.x * pComp.velocity.x + pComp.velocity.z * pComp.velocity.z;
        bool isMoving = !isDead && !isHurt && !isJumping && (speedSq > 0.1f);
        bool isAttacking = !isDead && !isHurt && (pComp.currentActionType > 0);

        float attackT = 0.0f;
        if (isAttacking) {
            float maxTime = (pComp.currentActionType == 2 || pComp.currentActionType == 4) ? 1.0f : 0.5f;
            attackT = std::clamp(pComp.actionTimer / maxTime, 0.0f, 1.0f);
        }

        float t = timeAccumulator;

        // ---------------------------------------------------------
        // y‘Ò‹@EˆÚ“® (ƒx[ƒXƒAƒjƒ[ƒVƒ‡ƒ“)z
        // ---------------------------------------------------------
        if (!isDead && !isJumping) {
            std::string animName = isMoving ? "Move" : "Idle";
            float duration = AnimationManager::GetInstance()->GetDuration(animName);
            float currentTime = fmodf(t, duration);

            state.rightShoulderRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ShoulderRight, currentTime);
            state.leftShoulderRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ShoulderLeft, currentTime);
            state.rightArmRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ArmRight, currentTime);
            state.leftArmRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ArmLeft, currentTime);
            state.rightLegRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::LegRight, currentTime);
            state.leftLegRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::LegLeft, currentTime);
            state.waistRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::Waist, currentTime);
            state.bodyRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::Body, currentTime);
            state.headRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::Head, currentTime);

            if (isMoving && pComp.isDashing && !isAttacking) {
                float dashPitch = 0.25f;
                state.waistRot = XMVectorAdd(state.waistRot, XMVectorSet(dashPitch, 0, 0, 0));
                state.bodyRot = XMVectorAdd(state.bodyRot, XMVectorSet(dashPitch * 0.2f, 0, 0, 0));
                state.headRot = XMVectorAdd(state.headRot, XMVectorSet(-dashPitch * 1.2f, 0, 0, 0));
                state.rightShoulderRot = XMVectorAdd(state.rightShoulderRot, XMVectorSet(dashPitch, 0, 0, 0));
                state.leftShoulderRot = XMVectorAdd(state.leftShoulderRot, XMVectorSet(dashPitch, 0, 0, 0));
                state.rightLegRot = XMVectorAdd(state.rightLegRot, XMVectorSet(dashPitch * 0.8f, 0, 0, 0));
                state.leftLegRot = XMVectorAdd(state.leftLegRot, XMVectorSet(dashPitch * 0.8f, 0, 0, 0));
            }
            state.waistOffset = XMVectorSet(0, sinf(t * (isMoving ? 10.0f : 1.5f)) * 0.05f, 0, 0);
        }

        // ---------------------------------------------------------
        // yUŒ‚ (ã”¼gƒuƒŒƒ“ƒh‘Î‰ž)z
        // ---------------------------------------------------------
        if (isAttacking) {
            float normalizedTime = 1.0f - attackT; // Ä¶•ûŒü

            if (pComp.currentActionType == 1 || pComp.currentActionType == 3) {
                // šŽËŒ‚: ã”¼g‚Ì‚ÝJSON‚Åã‘‚«‚µA‰º”¼g(‹rE˜)‚Íƒx[ƒX‚Ì“®‚«(‘–‚è“™)‚ðŽc‚·
                std::string animName = (pComp.currentActionType == 1) ? "ShootRight" : "ShootLeft";
                float duration = AnimationManager::GetInstance()->GetDuration(animName);
                float currentTime = normalizedTime * duration;

                state.rightShoulderRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ShoulderRight, currentTime);
                state.leftShoulderRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ShoulderLeft, currentTime);
                state.rightArmRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ArmRight, currentTime);
                state.leftArmRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ArmLeft, currentTime);
                state.bodyRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::Body, currentTime);
                state.headRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::Head, currentTime);
            }
            else if (pComp.currentActionType == 2 || pComp.currentActionType == 4) {
                // šŽaŒ‚: ‘«‚ðŽ~‚ß‚Ä‘Sg‚ðã‘‚«‚·‚é
                std::string animName = (pComp.currentActionType == 2) ? "AttackRight" : "AttackLeft";
                float duration = AnimationManager::GetInstance()->GetDuration(animName);
                float currentTime = normalizedTime * duration;

                state.rightShoulderRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ShoulderRight, currentTime);
                state.leftShoulderRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ShoulderLeft, currentTime);
                state.rightArmRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ArmRight, currentTime);
                state.leftArmRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ArmLeft, currentTime);
                state.waistRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::Waist, currentTime);
                state.bodyRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::Body, currentTime);
                state.headRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::Head, currentTime);

                float stanceT = 1.0f;
                if (attackT > 0.7f) stanceT = (1.0f - attackT) / 0.3f;
                else if (attackT < 0.2f) stanceT = attackT / 0.2f;

                if (pComp.currentActionType == 2) {
                    state.rightLegRot = XMVectorSet(Lerp(0.1f, 0.4f, stanceT), 0, 0, 0);
                    state.leftLegRot = XMVectorSet(Lerp(0.1f, -0.6f, stanceT), 0, 0, 0);
                }
                else {
                    state.leftLegRot = XMVectorSet(Lerp(0.1f, 0.4f, stanceT), 0, 0, 0);
                    state.rightLegRot = XMVectorSet(Lerp(0.1f, -0.6f, stanceT), 0, 0, 0);
                }

                if (attackT > 0.4f && attackT <= 0.7f) {
                    float subT = (0.7f - attackT) / 0.3f;
                    float easeOut = 1.0f - powf(1.0f - subT, 3.0f);
                    state.waistOffset = XMVectorSet(Lerp(0, 0, easeOut), Lerp(0.2f, -0.6f, easeOut), 0, 0);
                }
                else if (attackT <= 0.4f) {
                    float subT = (0.4f - attackT) / 0.4f;
                    float ease = 1.0f - powf(1.0f - subT, 2.0f);
                    state.waistOffset = XMVectorSet(0, Lerp(-0.6f, 0, ease), 0, 0);
                }
            }
        }

        // yƒWƒƒƒ“ƒvE‹ó’†z
        if (isJumping && !isAttacking) {
            float yVel = pComp.velocity.y;
            std::string animName = (yVel > 0) ? "JumpUp" : "JumpDown";
            state.rightShoulderRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ShoulderRight, 0);
            state.leftShoulderRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ShoulderLeft, 0);
            state.rightArmRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ArmRight, 0);
            state.leftArmRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ArmLeft, 0);
            state.rightLegRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::LegRight, 0);
            state.leftLegRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::LegLeft, 0);
            state.waistRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::Waist, 0);
            state.bodyRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::Body, 0);
            state.headRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::Head, 0);
        }

        // y”í’ez
        if (isHurt && !isDead) {
            std::string animName = "Hurt";
            state.waistRot = XMVectorAdd(state.waistRot, AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::Waist, 0));
            state.bodyRot = XMVectorAdd(state.bodyRot, AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::Body, 0));
            state.headRot = XMVectorAdd(state.headRot, AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::Head, 0));
            state.rightShoulderRot = XMVectorAdd(state.rightShoulderRot, AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ShoulderRight, 0));
            state.leftShoulderRot = XMVectorAdd(state.leftShoulderRot, AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ShoulderLeft, 0));
            float s = sinf(t * 25.0f) * 0.05f;
            state.waistOffset = XMVectorAdd(state.waistOffset, XMVectorSet(s, s, 0, 0));
        }

        // yŽ€–Sz
        if (isDead) {
            std::string animName = "Dead";
            state.waistRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::Waist, 0);
            state.bodyRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::Body, 0);
            state.headRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::Head, 0);
            state.rightShoulderRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ShoulderRight, 0);
            state.leftShoulderRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ShoulderLeft, 0);
            state.rightArmRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ArmRight, 0);
            state.leftArmRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ArmLeft, 0);
            state.rightLegRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::LegRight, 0);
            state.leftLegRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::LegLeft, 0);
            state.waistOffset = XMVectorSet(0, -1.2f, 0, 0);
        }

        animStates[coreID] = state;
    }

    std::unordered_map<EntityID, XMMATRIX> worldMatrixCache;
    std::function<EntityID(EntityID)> FindRootCore = [&](EntityID id) -> EntityID {
        if (partToCoreMap.count(id)) return partToCoreMap[id];
        if (!registry->HasComponent<PlayerPartComponent>(id)) return id;
        EntityID parent = (EntityID)registry->GetComponent<PlayerPartComponent>(id).parentID;
        EntityID root = FindRootCore(parent);
        partToCoreMap[id] = root;
        return root;
        };

    std::function<XMMATRIX(EntityID)> GetWorldMatrix = [&](EntityID id) -> XMMATRIX {
        if (worldMatrixCache.count(id)) return worldMatrixCache[id];
        if (!registry->HasComponent<TransformComponent>(id)) return XMMatrixIdentity();

        auto& trans = registry->GetComponent<TransformComponent>(id);
        XMMATRIX localMat = XMMatrixIdentity();

        if (registry->HasComponent<PlayerPartComponent>(id)) {
            auto& part = registry->GetComponent<PlayerPartComponent>(id);
            EntityID coreID = FindRootCore(id);

            XMVECTOR animRot = XMVectorZero();
            XMVECTOR animOffset = XMVectorZero();

            if (part.partModelID == -1 && animStates.count(coreID)) {
                AnimState& state = animStates[coreID];
                switch (part.partType) {
                case PartType::Waist:         animRot = state.waistRot; animOffset = state.waistOffset; break;
                case PartType::Body:          animRot = state.bodyRot; break;
                case PartType::Head:          animRot = state.headRot; break;
                case PartType::ShoulderRight: animRot = state.rightShoulderRot; break;
                case PartType::ArmRight:      animRot = state.rightArmRot; break;
                case PartType::ShoulderLeft:  animRot = state.leftShoulderRot; break;
                case PartType::ArmLeft:       animRot = state.leftArmRot; break;
                case PartType::LegRight:      animRot = state.rightLegRot; break;
                case PartType::LegLeft:       animRot = state.leftLegRot; break;
                default: break;
                }
            }

            XMMATRIX localS = XMMatrixScaling(trans.scale.x, trans.scale.y, trans.scale.z);
            XMMATRIX localR = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&part.baseRotation) + animRot);
            XMMATRIX localT = XMMatrixTranslationFromVector(XMLoadFloat3(&part.baseOffset) + animOffset);
            localMat = localS * localR * localT;

            if (part.parentID != -1 && part.parentID != id) {
                XMMATRIX parentMat = GetWorldMatrix((EntityID)part.parentID);
                localMat = localMat * parentMat;
            }
        }
        else {
            localMat = XMMatrixScaling(trans.scale.x, trans.scale.y, trans.scale.z) *
                XMMatrixRotationRollPitchYaw(trans.rotation.x, trans.rotation.y, trans.rotation.z) *
                XMMatrixTranslation(trans.position.x, trans.position.y, trans.position.z);
        }

        worldMatrixCache[id] = localMat;
        return localMat;
        };

    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (!registry->HasComponent<PlayerPartComponent>(id)) continue;
        if (!registry->HasComponent<TransformComponent>(id)) continue;

        XMMATRIX finalWorldMat = GetWorldMatrix(id);
        auto& partTrans = registry->GetComponent<TransformComponent>(id);

        XMVECTOR scale, rotQuat, trans;
        XMMatrixDecompose(&scale, &rotQuat, &trans, finalWorldMat);
        XMStoreFloat3(&partTrans.position, trans);

        XMMATRIX pureRotMat = XMMatrixRotationQuaternion(rotQuat);
        partTrans.rotation = ExtractEulerAngles(pureRotMat);
    }
}