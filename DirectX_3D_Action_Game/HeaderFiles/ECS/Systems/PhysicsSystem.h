#pragma once
#include "ECS/System.h"
#include "ECS/ECS.h"
#include <DirectXMath.h>

struct OBB {
	DirectX::XMFLOAT3 center;//中心座標
	DirectX::XMFLOAT3 extents;//半サイズ
	DirectX::XMFLOAT4X4 orientation;//回転（幅・高さ・奥行きの半分）
    DirectX::XMFLOAT4X4 worldMatrix;
};
class PhysicsSystem : public System {
public:
    // 更新処理
    void Update(float dt) override;
    OBB GetOBB(EntityID id);
private:
    // 衝突判定と解決の関数
    void CheckAndResolve(EntityID playerID, EntityID otherID);

    void CheckAttackHit(EntityID attackID, EntityID targetID);
    void CheckRecoveryHit(EntityID, EntityID);
    void CheckAttackSphereHit(EntityID attackID, EntityID targetID);
    void CheckRecoverySphereHit(EntityID recoveryID, EntityID targetID);
};