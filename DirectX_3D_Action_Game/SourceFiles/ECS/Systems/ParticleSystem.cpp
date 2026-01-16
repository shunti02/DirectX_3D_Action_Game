#include "ECS/Systems/ParticleSystem.h"
#include "ECS/World.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/ParticleComponent.h"

void ParticleSystem::Init(World* world) {
    pWorld = world;
}

void ParticleSystem::Update(float dt) {
    auto registry = pWorld->GetRegistry();

    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (!registry->HasComponent<ParticleComponent>(id)) continue;
        if (!registry->HasComponent<TransformComponent>(id)) continue;

        auto& p = registry->GetComponent<ParticleComponent>(id);
        auto& t = registry->GetComponent<TransformComponent>(id);

        // 1. 寿命管理
        p.lifeTime -= dt;
        if (p.lifeTime <= 0.0f) {
            pWorld->DestroyEntity(id); // 寿命が尽きたら消す
            continue;
        }

        // 2. 移動 (物理演算)
        t.position.x += p.velocity.x * dt;
        t.position.y += p.velocity.y * dt;
        t.position.z += p.velocity.z * dt;

        // 重力
        if (p.useGravity) {
            p.velocity.y -= 9.8f * dt;
        }

        // 3. サイズ変化 (徐々に小さくする等)
        if (p.scaleSpeed != 0.0f) {
            float s = t.scale.x + p.scaleSpeed * dt;
            if (s < 0.0f) s = 0.0f;
            t.scale = { s, s, s };
        }

        // 4. 回転 (適当に回す)
        t.rotation.x += 5.0f * dt;
        t.rotation.y += 3.0f * dt;
    }
}