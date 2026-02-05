#include "ECS/Systems/MovingSystem.h"
#include "ECS/World.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MovingComponent.h"
#include <cmath>

void MovingSystem::Update(float dt) {
    auto registry = pWorld->GetRegistry();

    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (!registry->HasComponent<MovingComponent>(id)) continue;
        if (!registry->HasComponent<TransformComponent>(id)) continue;

        auto& move = registry->GetComponent<MovingComponent>(id);
        auto& trans = registry->GetComponent<TransformComponent>(id);

        // 時間を進める
        move.time += dt * move.speed;

        // サイン波で往復 (-1.0 ~ 1.0)
        float s = sinf(move.time);

        // 新しい位置 = 基準位置 + (移動ベクトル * sin値)
        trans.position.x = move.startPos.x + move.moveVec.x * s;
        trans.position.y = move.startPos.y + move.moveVec.y * s;
        trans.position.z = move.startPos.z + move.moveVec.z * s;
    }
}