#pragma once
#include "ECS/System.h"
#include "ECS/ECS.h"
#include <DirectXMath.h>

struct OBB {
	DirectX::XMFLOAT3 center;
	DirectX::XMFLOAT3 extents;
	DirectX::XMFLOAT4X4 orientation;
    DirectX::XMFLOAT4X4 worldMatrix;
};
class PhysicsSystem : public System {
public:
    void Update(float dt) override;
    OBB GetOBB(EntityID id);
private:
    void CheckAndResolve(EntityID playerID, EntityID otherID);

    void CheckAttackHit(EntityID attackID, EntityID targetID);
    void CheckRecoveryHit(EntityID, EntityID);
    void CheckAttackSphereHit(EntityID attackID, EntityID targetID);
    void CheckBulletHit(EntityID bulletID, EntityID targetID);
};