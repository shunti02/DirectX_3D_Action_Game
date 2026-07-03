#define NOMINMAX
#include "ECS/Systems/EnemyAnimationSystem.h"
#include "ECS/World.h"
#include "ECS/Components/EnemyPartComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/EnemyComponent.h"
#include "ECS/Components/StatusComponent.h"
#include "ECS/Components/PhysicsComponent.h"
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

struct EnemyAnimState {
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

void EnemyAnimationSystem::Update(float dt) {
    timeAccumulator += dt;
    auto registry = pWorld->GetRegistry();

    std::unordered_map<EntityID, EnemyAnimState> animStates;
    std::unordered_map<EntityID, EntityID> partToCoreMap;

    for (EntityID coreID = 0; coreID < ECSConfig::MAX_ENTITIES; ++coreID) {
        if (!registry->HasComponent<EnemyComponent>(coreID) || !registry->HasComponent<TransformComponent>(coreID)) continue;

        auto& enemy = registry->GetComponent<EnemyComponent>(coreID);
        EnemyAnimState state;

        bool isDead = false;
        bool isHurt = false;
        if (registry->HasComponent<StatusComponent>(coreID)) {
            auto& status = registry->GetComponent<StatusComponent>(coreID);
            isDead = status.hp <= 0;
            isHurt = !isDead && (enemy.knockbackTimer > 0.0f);
        }

        float speedSq = 0.0f;
        if (registry->HasComponent<PhysicsComponent>(coreID)) {
            auto& phy = registry->GetComponent<PhysicsComponent>(coreID);
            speedSq = phy.velocity.x * phy.velocity.x + phy.velocity.z * phy.velocity.z;
        }
        bool isMoving = !isDead && !isHurt && (speedSq > 0.1f);
        bool isAttacking = !isDead && !isHurt && (enemy.state == EnemyState::Attack || enemy.state == EnemyState::Cooldown);

        float attackT = 0.0f;
        if (isAttacking) {
            if (enemy.attackDuration > 0.0f) {
                attackT = std::clamp(enemy.attackTimer / enemy.attackDuration, 0.0f, 1.0f);
            }
            if (enemy.state == EnemyState::Cooldown) attackT = 0.0f;
        }

        float t = timeAccumulator;

        // ---------------------------------------------------------
        // y‘Ò‹@EˆÚ“® (ƒx[ƒXƒAƒjƒ[ƒVƒ‡ƒ“)z
        // ---------------------------------------------------------
        if (!isDead) {
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

            if (isMoving && enemy.type == EnemyType::Normal) {
                float dashPitch = 0.35f;
                state.waistRot = XMVectorAdd(state.waistRot, XMVectorSet(dashPitch, 0, 0, 0));
                state.bodyRot = XMVectorAdd(state.bodyRot, XMVectorSet(dashPitch * 0.2f, 0, 0, 0));
                state.headRot = XMVectorAdd(state.headRot, XMVectorSet(-dashPitch * 1.2f, 0, 0, 0));
            }
            state.waistOffset = XMVectorSet(0, sinf(t * (isMoving ? 10.0f : 1.5f)) * 0.05f, 0, 0);
        }

        // ---------------------------------------------------------
        // yUŒ‚ (ƒuƒŒƒ“ƒh‘Î‰ž)z
        // ---------------------------------------------------------
        if (isAttacking) {
            std::string animName = (enemy.type == EnemyType::Ranged || enemy.type == EnemyType::Heavy) ? "ShootRight" : "AttackRight";
            float duration = AnimationManager::GetInstance()->GetDuration(animName);
            float currentTime = (1.0f - attackT) * duration;

            if (enemy.type == EnemyType::Normal) {
                // ‹ßÚƒƒ{: ‘Sgã‘‚«
                state.rightShoulderRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ShoulderRight, currentTime);
                state.leftShoulderRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ShoulderLeft, currentTime);
                state.rightArmRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ArmRight, currentTime);
                state.leftArmRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ArmLeft, currentTime);
                state.waistRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::Waist, currentTime);
                state.bodyRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::Body, currentTime);
                state.headRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::Head, currentTime);

                float stanceT = std::clamp((1.0f - attackT) * 2.0f, 0.0f, 1.0f);
                state.rightLegRot = XMVectorSet(0.1f + stanceT * 0.3f, 0, 0, 0);
                state.leftLegRot = XMVectorSet(0.1f - stanceT * 0.7f, 0, 0, 0);
            }
            else {
                // ‰“‹——£ƒƒ{: ã”¼g‚Ì‚ÝJSON‚Åã‘‚«‚µA‰º”¼g(ƒJƒj•à‚«ˆÚ“®)‚ðŽc‚·
                state.rightShoulderRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ShoulderRight, currentTime);
                state.leftShoulderRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ShoulderLeft, currentTime);
                state.rightArmRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ArmRight, currentTime);
                state.leftArmRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ArmLeft, currentTime);
                state.bodyRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::Body, currentTime);
                state.headRot = AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::Head, currentTime);
            }
        }

        // y”í’ez
        if (isHurt && !isDead) {
            std::string animName = "Hurt";
            state.waistRot = XMVectorAdd(state.waistRot, AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::Waist, 0));
            state.bodyRot = XMVectorAdd(state.bodyRot, AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::Body, 0));
            state.headRot = XMVectorAdd(state.headRot, AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::Head, 0));
            state.rightShoulderRot = XMVectorAdd(state.rightShoulderRot, AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ShoulderRight, 0));
            state.leftShoulderRot = XMVectorAdd(state.leftShoulderRot, AnimationManager::GetInstance()->EvaluateTrack(animName, PartType::ShoulderLeft, 0));
            state.waistOffset = XMVectorAdd(state.waistOffset, XMVectorSet(0, sinf(t * 25.0f) * 0.05f, 0, 0));
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

    // 2. FKŒvŽZ‚ÆTransform‚Ö‚Ì“K—p
    std::unordered_map<EntityID, XMMATRIX> worldMatrixCache;
    std::function<EntityID(EntityID)> FindRootCore = [&](EntityID id) -> EntityID {
        if (partToCoreMap.count(id)) return partToCoreMap[id];
        if (!registry->HasComponent<EnemyPartComponent>(id)) return id;
        EntityID parent = (EntityID)registry->GetComponent<EnemyPartComponent>(id).parentID;
        EntityID root = FindRootCore(parent);
        partToCoreMap[id] = root;
        return root;
        };

    std::function<XMMATRIX(EntityID)> GetWorldMatrix = [&](EntityID id) -> XMMATRIX {
        if (worldMatrixCache.count(id)) return worldMatrixCache[id];
        if (!registry->HasComponent<TransformComponent>(id)) return XMMatrixIdentity();

        auto& trans = registry->GetComponent<TransformComponent>(id);
        XMMATRIX localMat = XMMatrixIdentity();

        if (registry->HasComponent<EnemyPartComponent>(id)) {
            auto& part = registry->GetComponent<EnemyPartComponent>(id);
            EntityID coreID = FindRootCore(id);

            XMVECTOR animRot = XMVectorZero();
            XMVECTOR animOffset = XMVectorZero();

            if (part.partModelID == -1 && animStates.count(coreID)) {
                EnemyAnimState& state = animStates[coreID];
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
        if (!registry->HasComponent<EnemyPartComponent>(id)) continue;
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