#define NOMINMAX 

#include "App/Main.h"
#include "App/Game.h"
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
#include "ECS/Components/PhysicsComponent.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include "Game/EntityFactory.h"

using namespace DirectX;

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

static void DestroyEnemyParts(World* world, EntityID parentID) {
    auto registry = world->GetRegistry();
    std::vector<EntityID> partsToDelete;

    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (registry->HasComponent<EnemyPartComponent>(id)) {
            auto& part = registry->GetComponent<EnemyPartComponent>(id);
            if (part.parentID == (int)parentID) {
                partsToDelete.push_back(id);
            }
        }
    }

    for (EntityID id : partsToDelete) {
        world->DestroyEntity(id);
    }
}

// OBB
OBB PhysicsSystem::GetOBB(EntityID id) {
    auto registry = pWorld->GetRegistry();
    OBB obb = {};
    XMStoreFloat4x4(&obb.worldMatrix, XMMatrixIdentity());
    obb.center = { 0,0,0 };
    obb.extents = { 0.5f,0.5f,0.5f };

    if (!registry->HasComponent<TransformComponent>(id) ||
        !registry->HasComponent<ColliderComponent>(id)) {
        return obb;
    }

    const auto& trans = registry->GetComponent<TransformComponent>(id);
    const auto& col = registry->GetComponent<ColliderComponent>(id);
    if (col.type == ColliderType::Type_None) {
        obb.extents = { 0,0,0 };
        return obb;
    }

    XMMATRIX R = XMMatrixRotationRollPitchYaw(trans.rotation.x, trans.rotation.y, trans.rotation.z);
    XMMATRIX T = XMMatrixTranslation(trans.position.x, trans.position.y, trans.position.z);
    XMMATRIX world = R * T;

    XMStoreFloat4x4(&obb.worldMatrix, world);
    obb.center = trans.position;

    if (registry->HasComponent<PlayerComponent>(id) || registry->HasComponent<EnemyComponent>(id)) {
        obb.center.y += 1.0f;
        world.r[3] = XMVectorSet(obb.center.x, obb.center.y, obb.center.z, 1.0f);
        XMStoreFloat4x4(&obb.worldMatrix, world);
    }
    if (col.type == ColliderType::Type_Box) {
        obb.extents = {
            col.size.x * trans.scale.x * 0.5f,
            col.size.y * trans.scale.y * 0.5f,
            col.size.z * trans.scale.z * 0.5f
        };
    }
    else if (col.type == ColliderType::Type_Sphere) {
        float scaledRadius = col.radius * trans.scale.x;
        obb.extents = { scaledRadius, scaledRadius, scaledRadius };
    }
    else {
        float scaledRadius = col.radius * trans.scale.x;
        float scaledHeight = col.height * trans.scale.y;
        obb.extents = { scaledRadius, scaledHeight * 0.5f, scaledRadius };
    }
    return obb;
}

static bool RaycastGround(World* world, XMVECTOR origin, float maxDist, float& outDist) {
    auto registry = world->GetRegistry();
    float closestDist = maxDist;
    bool hitAny = false;
    XMVECTOR dirDown = XMVectorSet(0, -1, 0, 0);

    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (!registry->HasComponent<ColliderComponent>(id)) continue;
        if (!registry->HasComponent<TransformComponent>(id)) continue;

        if (registry->HasComponent<PlayerComponent>(id)) continue;
        if (registry->HasComponent<EnemyComponent>(id)) continue;
        if (registry->HasComponent<AttackBoxComponent>(id)) continue;
        if (registry->HasComponent<RecoveryBoxComponent>(id)) continue;
        if (registry->HasComponent<AttackSphereComponent>(id)) continue;
        if (registry->HasComponent<RecoverySphereComponent>(id)) continue;
        if (registry->HasComponent<PlayerPartComponent>(id)) continue;

        auto& trans = registry->GetComponent<TransformComponent>(id);
        if (trans.scale.y > 1.5f) continue;
        auto& col = registry->GetComponent<ColliderComponent>(id);
        if (col.type == ColliderType::Type_None) continue;

        BoundingOrientedBox obb;
        obb.Center = trans.position;
        obb.Extents = {
            col.size.x * trans.scale.x * 0.5f,
            col.size.y * trans.scale.y * 0.5f,
            col.size.z * trans.scale.z * 0.5f
        };
        XMVECTOR Q = XMQuaternionRotationRollPitchYaw(trans.rotation.x, trans.rotation.y, trans.rotation.z);
        XMStoreFloat4(&obb.Orientation, Q);

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

void PhysicsSystem::Update(float dt) {
    auto registry = pWorld->GetRegistry();

    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (registry->HasComponent<StatusComponent>(id)) {
            auto& status = registry->GetComponent<StatusComponent>(id);
            if (status.invincibleTimer > 0.0f) {
                status.invincibleTimer -= dt;
            }
        }
    }

    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (!registry->HasComponent<PhysicsComponent>(id)) continue;
        if (!registry->HasComponent<TransformComponent>(id)) continue;

        auto& phy = registry->GetComponent<PhysicsComponent>(id);
        auto& trans = registry->GetComponent<TransformComponent>(id);

        trans.position.x += phy.velocity.x * dt;
        trans.position.y += phy.velocity.y * dt;
        trans.position.z += phy.velocity.z * dt;

        if (phy.useGravity) {
            phy.velocity.y -= 9.8f * dt;
        }

        if (registry->HasComponent<PlayerComponent>(id) || registry->HasComponent<EnemyComponent>(id)) {
            float maxRadius = 38.0f;
            float distSq = trans.position.x * trans.position.x + trans.position.z * trans.position.z;

            if (distSq > maxRadius * maxRadius) {
                float dist = std::sqrt(distSq);

                float nx = trans.position.x / dist;
                float nz = trans.position.z / dist;

                trans.position.x = nx * maxRadius;
                trans.position.z = nz * maxRadius;

                XMVECTOR vel = XMLoadFloat3(&phy.velocity);
                XMVECTOR outNormal = XMVectorSet(nx, 0.0f, nz, 0.0f);
                float dot = XMVectorGetX(XMVector3Dot(vel, outNormal));

                if (dot > 0.0f) {
                    vel = vel - outNormal * dot;
                    XMStoreFloat3(&phy.velocity, vel);
                }
            }
        }

        if (!registry->HasComponent<PlayerComponent>(id) &&
            registry->HasComponent<ColliderComponent>(id) &&
            !registry->HasComponent<BulletComponent>(id))
        {
            float halfHeight = 0.5f * trans.scale.y;
            float rayDist = 0.0f;
            XMVECTOR origin = XMLoadFloat3(&trans.position);

            bool hit = RaycastGround(pWorld, origin, halfHeight + 0.5f, rayDist);

            if (hit) {
                if (rayDist <= halfHeight + 0.1f) {
                    float groundY = trans.position.y - rayDist;
                    trans.position.y = groundY + halfHeight;
                    if (phy.velocity.y < 0) {
                        phy.velocity.y = 0;
                    }
                    phy.velocity.x *= 0.9f;
                    phy.velocity.z *= 0.9f;
                    if (std::abs(phy.velocity.x) < 0.1f) phy.velocity.x = 0;
                    if (std::abs(phy.velocity.z) < 0.1f) phy.velocity.z = 0;
                }
            }
            else {
                if (trans.position.y < halfHeight) {
                    trans.position.y = halfHeight;
                    if (phy.velocity.y < 0) phy.velocity.y = 0;
                    phy.velocity.x *= 0.9f;
                    phy.velocity.z *= 0.9f;
                }
            }
        }
    }

    //è’ìÀîªíË
    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (!registry->HasComponent<EnemyComponent>(id)) continue;
        if (!registry->HasComponent<ColliderComponent>(id)) continue;

        for (EntityID otherID = 0; otherID < ECSConfig::MAX_ENTITIES; ++otherID) {
            if (id == otherID) continue;
            if (!registry->HasComponent<ColliderComponent>(otherID)) continue;
            if (registry->HasComponent<BulletComponent>(otherID)) continue;
            if (registry->HasComponent<AttackBoxComponent>(otherID)) continue;
            if (registry->HasComponent<AttackSphereComponent>(otherID)) continue;
            if (registry->HasComponent<EnemyComponent>(otherID)) continue;

            CheckAndResolve(id, otherID);
        }
    }

    // ÉvÉåÉCÉÑÅ[ÇÃï®óùãììÆ
    for (EntityID playerID = 0; playerID < ECSConfig::MAX_ENTITIES; ++playerID) {
        if (!registry->HasComponent<PlayerComponent>(playerID)) continue;
        if (!registry->HasComponent<ColliderComponent>(playerID)) continue;

        auto& pComp = registry->GetComponent<PlayerComponent>(playerID);
        auto& pTrans = registry->GetComponent<TransformComponent>(playerID);

        float hoverHeight = 1.0f;
        float rayDist = 0.0f;
        XMVECTOR rayOrigin = XMLoadFloat3(&pTrans.position);

        bool hitGround = RaycastGround(pWorld, rayOrigin, hoverHeight + 1.0f, rayDist);

        if (hitGround && rayDist <= hoverHeight) {
            pComp.isGrounded = true;
            float groundY = pTrans.position.y - rayDist;
            float targetY = groundY + hoverHeight;
            pTrans.position.y = targetY;

            auto& phy = registry->GetComponent<PhysicsComponent>(playerID);
            if (phy.velocity.y < 0) {
                phy.velocity.y = 0;
            }
        }
        else {
            pComp.isGrounded = false;
        }

        for (EntityID otherID = 0; otherID < ECSConfig::MAX_ENTITIES; ++otherID) {
            if (playerID == otherID) continue;
            if (!registry->HasComponent<ColliderComponent>(otherID)) continue;
            CheckAndResolve(playerID, otherID);
        }
    }

    //çUåÇîªíË
    for (EntityID attackID = 0; attackID < ECSConfig::MAX_ENTITIES; ++attackID) {
        if (!registry->HasComponent<AttackBoxComponent>(attackID)) continue;
        auto& attackBox = registry->GetComponent<AttackBoxComponent>(attackID);
        EntityID ownerID = attackBox.ownerID;

        for (EntityID targetID = 0; targetID < ECSConfig::MAX_ENTITIES; ++targetID) {
            if (attackID == targetID) continue;
            if (targetID == ownerID) continue;
            if (!registry->HasComponent<ColliderComponent>(targetID)) continue;
            if (!registry->HasComponent<StatusComponent>(targetID)) continue;
            CheckAttackHit(attackID, targetID);
        }
    }
    //çUåÇãÖîªíË
    for (EntityID attackID = 0; attackID < ECSConfig::MAX_ENTITIES; ++attackID) {
        if (!registry->HasComponent<AttackSphereComponent>(attackID)) continue;
        auto& sphere = registry->GetComponent<AttackSphereComponent>(attackID);
        EntityID ownerID = sphere.ownerID;

        for (EntityID targetID = 0; targetID < ECSConfig::MAX_ENTITIES; ++targetID) {
            if (attackID == targetID) continue;
            if (targetID == ownerID) continue;
            if (!registry->HasComponent<ColliderComponent>(targetID)) continue;
            if (!registry->HasComponent<StatusComponent>(targetID)) continue;
            CheckAttackSphereHit(attackID, targetID);
        }
    }

    //íeîªíË
    for (EntityID bulletID = 0; bulletID < ECSConfig::MAX_ENTITIES; ++bulletID) {
        if (!registry->HasComponent<BulletComponent>(bulletID)) continue;
        auto& bullet = registry->GetComponent<BulletComponent>(bulletID);
        if (!bullet.isActive) continue;

        if (bullet.fromPlayer) {
            for (EntityID targetID = 0; targetID < ECSConfig::MAX_ENTITIES; ++targetID) {
                if (bulletID == targetID) continue;
                if (!registry->HasComponent<EnemyComponent>(targetID)) continue;
                if (!registry->HasComponent<ColliderComponent>(targetID)) continue;
                if (!registry->HasComponent<StatusComponent>(targetID)) continue;

                CheckBulletHit(bulletID, targetID);
                if (!bullet.isActive) break;
            }
        }
        else {
            for (EntityID targetID = 0; targetID < ECSConfig::MAX_ENTITIES; ++targetID) {
                if (bulletID == targetID) continue;
                if (!registry->HasComponent<PlayerComponent>(targetID)) continue;
                if (!registry->HasComponent<ColliderComponent>(targetID)) continue;
                if (!registry->HasComponent<StatusComponent>(targetID)) continue;

                CheckBulletHit(bulletID, targetID);
                if (!bullet.isActive) break;
            }
        }
    }
}
//è’ìÀîªíËÇÃé¿ëï
void PhysicsSystem::CheckAndResolve(EntityID entityID, EntityID otherID) {
    auto registry = pWorld->GetRegistry();

    if (registry->HasComponent<AttackBoxComponent>(otherID)) return;
    if (registry->HasComponent<RecoveryBoxComponent>(otherID)) return;
    if (registry->HasComponent<AttackSphereComponent>(otherID)) return;
    if (registry->HasComponent<RecoverySphereComponent>(otherID)) return;
    if (registry->HasComponent<BulletComponent>(otherID)) return;

    if (registry->HasComponent<PlayerPartComponent>(otherID)) return;
    if (registry->HasComponent<EnemyPartComponent>(otherID)) return;
    auto& otherCol = registry->GetComponent<ColliderComponent>(otherID);
    if (otherCol.type == ColliderType::Type_None) return;

    if (registry->HasComponent<PlayerPartComponent>(otherID)) {
        auto& part = registry->GetComponent<PlayerPartComponent>(otherID);
        if (part.parentID == (int)entityID) return;
    }

    auto& pTrans = registry->GetComponent<TransformComponent>(entityID);
    auto& pCol = registry->GetComponent<ColliderComponent>(entityID);
    auto& phy = registry->GetComponent<PhysicsComponent>(entityID);

    OBB boxOBB = GetOBB(otherID);

    XMMATRIX boxWorld = XMLoadFloat4x4(&boxOBB.worldMatrix);
    XMVECTOR det;
    XMMATRIX boxInvWorld = XMMatrixInverse(&det, boxWorld);

    float pRadius = pCol.radius * pTrans.scale.x;
    float pHeight = pCol.height * pTrans.scale.y;
    float cylinderLen = std::max<float>(0.0f, pHeight - 2.0f * pRadius);
    float halfLen = cylinderLen * 0.5f;
    XMVECTOR pos = XMLoadFloat3(&pTrans.position);
    XMVECTOR up = XMVectorSet(0, 1, 0, 0);
    XMVECTOR pStartW = pos - up * halfLen;
    XMVECTOR pEndW = pos + up * halfLen;

    float radius = pRadius;
    float maxPenetration = -1.0f;
    XMVECTOR finalPushW = XMVectorZero();
    bool isHit = false;

    XMVECTOR pStartL = XMVector3TransformCoord(pStartW, boxInvWorld);
    XMVECTOR pEndL = XMVector3TransformCoord(pEndW, boxInvWorld);
    float hx = boxOBB.extents.x;
    float hy = boxOBB.extents.y;
    float hz = boxOBB.extents.z;

    XMVECTOR segmentVec = pEndL - pStartL;
    float segLen = XMVectorGetX(XMVector3Length(segmentVec));
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

    XMFLOAT3 c[8] = {
        {-hx,-hy,-hz}, { hx,-hy,-hz}, {-hx, hy,-hz}, { hx, hy,-hz},
        {-hx,-hy, hz}, { hx,-hy, hz}, {-hx, hy, hz}, { hx, hy, hz}
    };
    XMVECTOR v[8];
    for (int i = 0; i < 8; ++i) v[i] = XMVector3TransformCoord(XMLoadFloat3(&c[i]), boxWorld);

    int edges[12][2] = {
        {0,1}, {2,3}, {4,5}, {6,7},
        {0,2}, {1,3}, {4,6}, {5,7},
        {0,4}, {1,5}, {2,6}, {3,7}
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
                pushDirW = XMVectorSet(0, 1.0f, 0, 0);
            }

            if (pen > maxPenetration) {
                maxPenetration = pen;
                finalPushW = pushDirW * pen;
                isHit = true;
            }
        }
    }

    //è’ìÀâåà
    if (isHit) {
        bool isTargetPlayer = registry->HasComponent<PlayerComponent>(otherID);
        if (!isTargetPlayer && registry->HasComponent<StatusComponent>(otherID)) {
            if (registry->HasComponent<StatusComponent>(entityID)) {
                auto& playerStatus = registry->GetComponent<StatusComponent>(entityID);
                auto& enemyStatus = registry->GetComponent<StatusComponent>(otherID);

                if (playerStatus.invincibleTimer <= 0.0f) {
                    int damage = (enemyStatus.attackPower > 0) ? enemyStatus.attackPower : 10;
                    playerStatus.TakeDamage(damage);
                    DebugLog("OUCH! Player Hit by Enemy! HP: %d", playerStatus.hp);

                    playerStatus.invincibleTimer = 1.0f;

                    XMVECTOR enemyPos = XMLoadFloat3(&boxOBB.center);
                    XMVECTOR myPos = pos;
                    XMVECTOR dir = myPos - enemyPos;
                    dir = XMVectorSetY(dir, 0.0f);

                    if (XMVectorGetX(XMVector3LengthSq(dir)) < 0.0001f) {
                        dir = XMVectorSet(0, 0, -1.0f, 0);
                    }
                    else {
                        dir = XMVector3Normalize(dir);
                    }

                    XMVECTOR knockbackVel = dir * 15.0f;
                    knockbackVel = XMVectorSetY(knockbackVel, 10.0f);

                    XMStoreFloat3(&phy.velocity, knockbackVel);

                    if (registry->HasComponent<PlayerComponent>(entityID)) {
                        registry->GetComponent<PlayerComponent>(entityID).isGrounded = false;
                    }
                }
            }
        }

        XMVECTOR currentPos = XMLoadFloat3(&pTrans.position);
        currentPos += finalPushW;
        XMStoreFloat3(&pTrans.position, currentPos);

        XMVECTOR pushDir;
        if (XMVectorGetX(XMVector3LengthSq(finalPushW)) < 0.000001f) {
            pushDir = XMVectorSet(0, 1.0f, 0, 0);
        }
        else {
            pushDir = XMVector3Normalize(finalPushW);
        }

        XMVECTOR vel = XMLoadFloat3(&phy.velocity);
        float dot = XMVectorGetX(XMVector3Dot(vel, pushDir));
        if (dot < 0.0f) {
            vel = vel - pushDir * dot;
            XMStoreFloat3(&phy.velocity, vel);
        }

        if (registry->HasComponent<PlayerComponent>(entityID)) {
            auto& pComp = registry->GetComponent<PlayerComponent>(entityID);
            if (XMVectorGetY(finalPushW) > 0.001f) {
                if (XMVectorGetY(pushDir) > 0.6f) {
                    pComp.isGrounded = true;
                }
            }
            else if (XMVectorGetY(finalPushW) < -0.001f) {
                if (XMVectorGetY(pushDir) < -0.6f && phy.velocity.y > 0) {
                    phy.velocity.y = 0;
                }
            }
        }
    }
}
// çUåÇîªíË
void PhysicsSystem::CheckAttackHit(EntityID attackID, EntityID targetID) {
    auto registry = pWorld->GetRegistry();

    if (registry->HasComponent<ColliderComponent>(targetID)) {
        if (registry->GetComponent<ColliderComponent>(targetID).type == ColliderType::Type_None) return;
    }

    auto& attackBox = registry->GetComponent<AttackBoxComponent>(attackID);
    EntityID ownerID = attackBox.ownerID;

    bool isOwnerPlayer = registry->HasComponent<PlayerComponent>(ownerID);
    bool isTargetPlayer = registry->HasComponent<PlayerComponent>(targetID);
    if (isOwnerPlayer == isTargetPlayer) return;

    auto& aTrans = registry->GetComponent<TransformComponent>(attackID);
    float radius = 0.5f * aTrans.scale.x;
    XMVECTOR attackPos = XMLoadFloat3(&aTrans.position);

    OBB targetOBB = GetOBB(targetID);
    XMMATRIX targetWorld = XMLoadFloat4x4(&targetOBB.worldMatrix);
    XMVECTOR det;
    XMMATRIX targetInvWorld = XMMatrixInverse(&det, targetWorld);

    XMVECTOR centerL = XMVector3TransformCoord(attackPos, targetInvWorld);
    XMFLOAT3 p; XMStoreFloat3(&p, centerL);

    float hx = targetOBB.extents.x;
    float hy = targetOBB.extents.y;
    float hz = targetOBB.extents.z;

    float cx = std::max<float>(-hx, std::min<float>(p.x, hx));
    float cy = std::max<float>(-hy, std::min<float>(p.y, hy));
    float cz = std::max<float>(-hz, std::min<float>(p.z, hz));

    float dx = p.x - cx;
    float dy = p.y - cy;
    float dz = p.z - cz;
    float distSq = dx * dx + dy * dy + dz * dz;

    if (distSq < radius * radius) {
        auto& targetStatus = registry->GetComponent<StatusComponent>(targetID);

        if (targetStatus.invincibleTimer <= 0.0f) {
            targetStatus.TakeDamage(attackBox.damage);
            targetStatus.invincibleTimer = 0.5f;
            if (registry->HasComponent<TransformComponent>(targetID)) {
                auto& tTrans = registry->GetComponent<TransformComponent>(targetID);
                EntityFactory::CreateHitEffect(pWorld, tTrans.position, 8, { 1.0f, 0.5f, 0.0f, 1.0f });
                if (targetStatus.IsDead()) {
                    EntityFactory::CreateHitEffect(pWorld, tTrans.position, 20, { 1.0f, 0.2f, 0.2f, 1.0f });
                }
            }

            if (registry->HasComponent<EnemyComponent>(targetID) &&
                registry->HasComponent<PhysicsComponent>(targetID))
            {
                auto& enemy = registry->GetComponent<EnemyComponent>(targetID);
                if (!enemy.isImmovable) {
                    auto& ePhy = registry->GetComponent<PhysicsComponent>(targetID);
                    auto& eTrans = registry->GetComponent<TransformComponent>(targetID);

                    XMVECTOR enemyPosVal = XMLoadFloat3(&eTrans.position);
                    XMVECTOR dir = enemyPosVal - attackPos;
                    dir = XMVectorSetY(dir, 0.0f);

                    if (XMVectorGetX(XMVector3LengthSq(dir)) < 0.0001f) {
                        dir = XMVectorSet(0, 0, 1.0f, 0);
                    }
                    else {
                        dir = XMVector3Normalize(dir);
                    }

                    float safeWeight = std::max(0.1f, enemy.weight);
                    float knockBackPower = 10.0f / safeWeight;
                    float liftPower = 5.0f / safeWeight;

                    XMVECTOR v = dir * knockBackPower;
                    v = XMVectorSetY(v, liftPower);
                    XMStoreFloat3(&ePhy.velocity, v);
                    enemy.knockbackTimer = 0.5f;
                }
            }

            if (isTargetPlayer) {
                auto& pTrans = registry->GetComponent<TransformComponent>(targetID);
                auto& phy = registry->GetComponent<PhysicsComponent>(targetID);

                XMVECTOR enemyPos;
                if (registry->HasComponent<TransformComponent>(ownerID)) {
                    enemyPos = XMLoadFloat3(&registry->GetComponent<TransformComponent>(ownerID).position);
                }
                else {
                    enemyPos = attackPos;
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

                XMVECTOR knockbackVel = dir * 10.0f;
                knockbackVel = XMVectorSetY(knockbackVel, 5.0f);

                XMStoreFloat3(&phy.velocity, knockbackVel);
                registry->GetComponent<PlayerComponent>(targetID).isGrounded = false;
            }

            if (targetStatus.IsDead()) {
                if (registry->HasComponent<TransformComponent>(targetID)) {
                    if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_SWITCH");
                }
                if (!isTargetPlayer) {
                    DestroyEnemyParts(pWorld, targetID);
                    pWorld->DestroyEntity(targetID);
                }
            }
        }
    }
}
//çUåÇãÖÇÃîªíË
void PhysicsSystem::CheckAttackSphereHit(EntityID attackID, EntityID targetID) {
    auto registry = pWorld->GetRegistry();

    if (registry->HasComponent<ColliderComponent>(targetID)) {
        if (registry->GetComponent<ColliderComponent>(targetID).type == ColliderType::Type_None) return;
    }

    auto& sphere = registry->GetComponent<AttackSphereComponent>(attackID);
    auto& trans = registry->GetComponent<TransformComponent>(attackID);
    EntityID ownerID = sphere.ownerID;

    bool isOwnerPlayer = registry->HasComponent<PlayerComponent>(sphere.ownerID);
    bool isTargetPlayer = registry->HasComponent<PlayerComponent>(targetID);
    if (isOwnerPlayer == isTargetPlayer) return;

    float radius = sphere.currentRadius;
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

            if (registry->HasComponent<TransformComponent>(targetID)) {
                auto& tTrans = registry->GetComponent<TransformComponent>(targetID);
                EntityFactory::CreateHitEffect(pWorld, tTrans.position, 5, { 1.0f, 0.8f, 0.0f, 1.0f });

                if (targetStatus.IsDead()) {
                    EntityFactory::CreateHitEffect(pWorld, tTrans.position, 20, { 1.0f, 0.2f, 0.2f, 1.0f });
                }
            }

            if (registry->HasComponent<EnemyComponent>(targetID) &&
                registry->HasComponent<PhysicsComponent>(targetID))
            {
                auto& enemy = registry->GetComponent<EnemyComponent>(targetID);
                if (!enemy.isImmovable) {
                    auto& ePhy = registry->GetComponent<PhysicsComponent>(targetID);
                    auto& eTrans = registry->GetComponent<TransformComponent>(targetID);

                    XMVECTOR ePos = XMLoadFloat3(&eTrans.position);
                    XMVECTOR dir = ePos - spherePos;
                    dir = XMVectorSetY(dir, 0.0f);

                    if (XMVectorGetX(XMVector3LengthSq(dir)) < 0.0001f) {
                        dir = XMVectorSet(0, 0, 1.0f, 0);
                    }
                    else {
                        dir = XMVector3Normalize(dir);
                    }

                    float safeWeight = std::max(0.1f, enemy.weight);
                    float knockBackPower = 15.0f / safeWeight;
                    float liftPower = 8.0f / safeWeight;

                    XMVECTOR v = dir * knockBackPower;
                    v = XMVectorSetY(v, liftPower);

                    XMStoreFloat3(&ePhy.velocity, v);
                    enemy.knockbackTimer = 0.5f;
                }
            }

            if (isTargetPlayer) {
                auto& pTrans = registry->GetComponent<TransformComponent>(targetID);
                auto& phy = registry->GetComponent<PhysicsComponent>(targetID);

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

                XMStoreFloat3(&phy.velocity, knockbackVel);
                registry->GetComponent<PlayerComponent>(targetID).isGrounded = false;
            }

            if (targetStatus.IsDead()) {
                if (registry->HasComponent<TransformComponent>(targetID)) {
                    if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_SWITCH");
                }
                if (!isTargetPlayer) {
                    DestroyEnemyParts(pWorld, targetID);
                    pWorld->DestroyEntity(targetID);
                }
            }
        }
    }
}
// íeÇÃîªíË
void PhysicsSystem::CheckBulletHit(EntityID bulletID, EntityID targetID) {
    auto registry = pWorld->GetRegistry();

    auto& bullet = registry->GetComponent<BulletComponent>(bulletID);
    auto& bTrans = registry->GetComponent<TransformComponent>(bulletID);
    XMVECTOR bulletPos = XMLoadFloat3(&bTrans.position);
    float bulletRadius = 0.3f;

    OBB targetOBB = GetOBB(targetID);
    XMMATRIX targetWorld = XMLoadFloat4x4(&targetOBB.worldMatrix);
    XMVECTOR det;
    XMMATRIX targetInvWorld = XMMatrixInverse(&det, targetWorld);

    XMVECTOR centerL = XMVector3TransformCoord(bulletPos, targetInvWorld);
    XMFLOAT3 p; XMStoreFloat3(&p, centerL);

    float hx = targetOBB.extents.x;
    float hy = targetOBB.extents.y;
    float hz = targetOBB.extents.z;

    float cx = std::max<float>(-hx, std::min<float>(p.x, hx));
    float cy = std::max<float>(-hy, std::min<float>(p.y, hy));
    float cz = std::max<float>(-hz, std::min<float>(p.z, hz));

    float dx = p.x - cx; float dy = p.y - cy; float dz = p.z - cz;
    float distSq = dx * dx + dy * dy + dz * dz;

    if (distSq < bulletRadius * bulletRadius) {
        auto& targetStatus = registry->GetComponent<StatusComponent>(targetID);

        if (registry->HasComponent<PlayerComponent>(targetID)) {
            if (targetStatus.invincibleTimer <= 0.0f) {
                targetStatus.TakeDamage(bullet.damage);
                targetStatus.invincibleTimer = 0.5f;

                EntityFactory::CreateHitEffect(pWorld, bTrans.position, 5, { 1.0f, 0.2f, 0.0f, 1.0f });

                auto& phy = registry->GetComponent<PhysicsComponent>(targetID);
                XMVECTOR knockDir;
                if (registry->HasComponent<PhysicsComponent>(bulletID)) {
                    auto& bPhy = registry->GetComponent<PhysicsComponent>(bulletID);
                    knockDir = XMLoadFloat3(&bPhy.velocity);
                }
                else {
                    knockDir = XMVectorSet(0, 0, 1, 0);
                }
                knockDir = XMVectorSetY(knockDir, 0.0f);

                if (XMVectorGetX(XMVector3LengthSq(knockDir)) < 0.0001f) {
                    knockDir = XMVectorSet(0, 0, 1.0f, 0);
                }
                else {
                    knockDir = XMVector3Normalize(knockDir);
                }

                XMVECTOR knockVel = knockDir * 8.0f;
                knockVel = XMVectorSetY(knockVel, 5.0f);

                XMStoreFloat3(&phy.velocity, knockVel);
                registry->GetComponent<PlayerComponent>(targetID).isGrounded = false;

                if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_SWITCH");

                bullet.isActive = false;
                pWorld->DestroyEntity(bulletID);
                return;
            }
        }
        else if (registry->HasComponent<EnemyComponent>(targetID)) {
            targetStatus.TakeDamage(bullet.damage);
            EntityFactory::CreateHitEffect(pWorld, bTrans.position, 5, { 0.0f, 1.0f, 1.0f, 1.0f });

            auto& enemy = registry->GetComponent<EnemyComponent>(targetID);
            if (!enemy.isImmovable && registry->HasComponent<PhysicsComponent>(targetID)) {
                auto& ePhy = registry->GetComponent<PhysicsComponent>(targetID);
                XMVECTOR knockDir;
                if (registry->HasComponent<PhysicsComponent>(bulletID)) {
                    auto& bPhy = registry->GetComponent<PhysicsComponent>(bulletID);
                    knockDir = XMLoadFloat3(&bPhy.velocity);
                }
                else {
                    knockDir = XMVectorSet(0, 0, 1, 0);
                }
                knockDir = XMVectorSetY(knockDir, 0.0f);

                if (XMVectorGetX(XMVector3LengthSq(knockDir)) < 0.0001f) {
                    knockDir = XMVectorSet(0, 0, 1.0f, 0);
                }
                else {
                    knockDir = XMVector3Normalize(knockDir);
                }

                float safeWeight = std::max(0.1f, enemy.weight);
                float knockPower = 20.0f / safeWeight;
                XMVECTOR v = knockDir * knockPower;
                v = XMVectorSetY(v, 2.0f / safeWeight);
                XMStoreFloat3(&ePhy.velocity, v);
                enemy.knockbackTimer = 0.2f;
            }

            if (targetStatus.IsDead()) {
                if (registry->HasComponent<TransformComponent>(targetID)) {
                    auto& tf = registry->GetComponent<TransformComponent>(targetID);
                    EntityFactory::CreateHitEffect(pWorld, tf.position, 20, { 1.0f, 0.2f, 0.2f, 1.0f });
                }
                DestroyEnemyParts(pWorld, targetID);
                pWorld->DestroyEntity(targetID);
                if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_SWITCH");
            }

            bullet.isActive = false;
            pWorld->DestroyEntity(bulletID);
            return;
        }
    }
}