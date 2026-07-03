#include "ECS/Systems/ParticleSystem.h"
#include "ECS/World.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/ParticleComponent.h"

void ParticleSystem::Init(World* world) {
    pWorld = world;
}

void ParticleSystem::Update(float dt) {
    auto registry = pWorld->GetRegistry();
    std::vector<EntityID> destroyList;

    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (!registry->HasComponent<ParticleComponent>(id)) continue;

        auto& p = registry->GetComponent<ParticleComponent>(id);
        auto& t = registry->GetComponent<TransformComponent>(id);

        p.lifeTime -= dt;
        if (p.lifeTime <= 0.0f) {
            destroyList.push_back(id);
            continue;
        }

        // ˆÚ“®
        t.position.x += p.velocity.x * dt;
        t.position.y += p.velocity.y * dt;
        t.position.z += p.velocity.z * dt;

        if (p.type == ParticleType::Spark) {
            if (p.useGravity) p.velocity.y -= 9.8f * dt;
            if (t.position.y < 0.0f) {
                t.position.y = 0.0f;
                p.velocity.y *= -0.5f;
            }
        }
        else if (p.type == ParticleType::Explosion) {
            p.velocity.x *= 0.95f;
            p.velocity.y *= 0.95f;
            p.velocity.z *= 0.95f;
        }
        else if (p.type == ParticleType::Smoke) {
            p.velocity.y += 1.0f * dt;
            p.velocity.x *= 0.9f;
            p.velocity.z *= 0.9f;
        }
        else if (p.type == ParticleType::MuzzleFlash) {
        }

        t.rotation.x += dt * 5.0f;
        t.rotation.z += dt * 5.0f;

        float ds = p.scaleSpeed * dt;
        t.scale.x += ds;
        t.scale.y += ds;
        t.scale.z += ds;

        if (t.scale.x < 0.0f) t.scale = { 0,0,0 };
    }

    for (auto id : destroyList) {
        pWorld->DestroyEntity(id);
    }
}