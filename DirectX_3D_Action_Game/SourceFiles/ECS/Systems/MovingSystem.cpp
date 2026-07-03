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

        move.time += dt * move.speed;

        float s = sinf(move.time);

        trans.position.x = move.startPos.x + move.moveVec.x * s;
        trans.position.y = move.startPos.y + move.moveVec.y * s;
        trans.position.z = move.startPos.z + move.moveVec.z * s;
    }
}