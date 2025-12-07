#pragma once
#include "ECS/System.h"
#include "ECS/ECS.h"

// クラスの宣言だけを行う
class PhysicsSystem : public System {
public:
    // 更新処理
    void Update(float dt) override;

private:
    // 衝突判定と解決の関数
    void CheckAndResolve(EntityID playerID, EntityID otherID);
};