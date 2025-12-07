#define NOMINMAX 

#include "ECS/Systems/PhysicsSystem.h"
#include "ECS/World.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/ColliderComponent.h"
#include "ECS/Components/PlayerComponent.h"
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
// Update関数の実装
// -----------------------------------------------------------------------
void PhysicsSystem::Update(float dt) {
    auto registry = pWorld->GetRegistry();

    EntityID playerID = ECSConfig::INVALID_ID;
    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (registry->HasComponent<PlayerComponent>(id)) {
            playerID = id; break;
        }
    }
    if (playerID == ECSConfig::INVALID_ID) return;

    auto& pComp = registry->GetComponent<PlayerComponent>(playerID);
    pComp.isGrounded = false;

    for (EntityID otherID = 0; otherID < ECSConfig::MAX_ENTITIES; ++otherID) {
        if (playerID == otherID) continue;
        if (!registry->HasComponent<ColliderComponent>(otherID)) continue;
        CheckAndResolve(playerID, otherID);
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

    auto& oTrans = registry->GetComponent<TransformComponent>(otherID);
    auto& oCol = registry->GetComponent<ColliderComponent>(otherID);

    // 行列計算
    XMMATRIX R = XMMatrixRotationRollPitchYaw(oTrans.rotation.x, oTrans.rotation.y, oTrans.rotation.z);
    XMMATRIX T = XMMatrixTranslation(oTrans.position.x, oTrans.position.y, oTrans.position.z);
    XMMATRIX boxWorld = R * T;
    XMVECTOR det;
    XMMATRIX boxInvWorld = XMMatrixInverse(&det, boxWorld);

    // カプセル芯
    float cylinderLen = std::max<float>(0.0f, pCol.height - 2.0f * pCol.radius);
    float halfLen = cylinderLen * 0.5f;
    XMVECTOR pos = XMLoadFloat3(&pTrans.position);
    XMVECTOR up = XMVectorSet(0, 1, 0, 0);
    XMVECTOR pStartW = pos - up * halfLen;
    XMVECTOR pEndW = pos + up * halfLen;

    float radius = pCol.radius;
    float maxPenetration = -1.0f;
    XMVECTOR finalPushW = XMVectorZero();
    bool isHit = false;

    // --- [判定A] カプセル線分 vs 箱 (ローカル空間) ---
    XMVECTOR pStartL = XMVector3TransformCoord(pStartW, boxInvWorld);
    XMVECTOR pEndL = XMVector3TransformCoord(pEndW, boxInvWorld);
    float hx = oCol.size.x * 0.5f;
    float hy = oCol.size.y * 0.5f;
    float hz = oCol.size.z * 0.5f;

    XMVECTOR segmentVec = pEndL - pStartL;
    float segLen = XMVectorGetX(XMVector3Length(segmentVec));
    int steps = static_cast<int>(segLen / (radius * 0.5f)) + 2;

    for (int i = 0; i < steps; ++i) {
        float t = (float)i / (steps - 1);
        XMVECTOR pointL = pStartL + segmentVec * t;
        XMFLOAT3 p; XMStoreFloat3(&p, pointL);

        float cx = std::max<float>(-hx, std::min<float>(p.x, hx));
        float cy = std::max<float>(-hy, std::min<float>(p.y, hy));
        float cz = std::max<float>(-hz, std::min<float>(p.z, hz));

        float dx = p.x - cx; float dy = p.y - cy; float dz = p.z - cz;
        float distSq = dx * dx + dy * dy + dz * dz;

        if (distSq < radius * radius) {
            float dist = std::sqrt(distSq);
            float pen = radius - dist;
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