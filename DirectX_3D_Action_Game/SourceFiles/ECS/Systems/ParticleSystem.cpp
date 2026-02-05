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

        // 移動
        t.position.x += p.velocity.x * dt;
        t.position.y += p.velocity.y * dt;
        t.position.z += p.velocity.z * dt;

        // タイプ別挙動
        if (p.type == ParticleType::Spark) {
            if (p.useGravity) p.velocity.y -= 9.8f * dt;
            // 地面で跳ねる簡易処理
            if (t.position.y < 0.0f) {
                t.position.y = 0.0f;
                p.velocity.y *= -0.5f; // 反発
            }
        }
        else if (p.type == ParticleType::Explosion) {
            // 減速
            p.velocity.x *= 0.95f;
            p.velocity.y *= 0.95f;
            p.velocity.z *= 0.95f;
        }
        // ★追加: 煙 (ふわふわ上昇)
        else if (p.type == ParticleType::Smoke) {
            p.velocity.y += 1.0f * dt; // 加速上昇
            p.velocity.x *= 0.9f;      // 横移動は減速
            p.velocity.z *= 0.9f;
        }
        // ★追加: マズルフラッシュ (特に移動処理なし、短命)
        else if (p.type == ParticleType::MuzzleFlash) {
            // その場に留まるか、親に追従すべきだが今回は簡易的に固定
        }

        // 回転
        t.rotation.x += dt * 5.0f;
        t.rotation.z += dt * 5.0f;

        // スケール変化
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

//void ParticleSystem::Update(float dt) {
//    auto registry = pWorld->GetRegistry();
//    
//    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
//        if (!registry->HasComponent<ParticleComponent>(id)) continue;
//        if (!registry->HasComponent<TransformComponent>(id)) continue;
//
//        auto& p = registry->GetComponent<ParticleComponent>(id);
//        auto& t = registry->GetComponent<TransformComponent>(id);
//
//        // 1. 寿命管理
//        p.lifeTime -= dt;
//        if (p.lifeTime <= 0.0f) {
//            pWorld->DestroyEntity(id); // 寿命が尽きたら消す
//            continue;
//        }
//
//        // 2. 移動 (物理演算)
//        t.position.x += p.velocity.x * dt;
//        t.position.y += p.velocity.y * dt;
//        t.position.z += p.velocity.z * dt;
//
//        // 重力
//        if (p.useGravity) {
//            p.velocity.y -= 9.8f * dt;
//        }
//
//        // 3. サイズ変化 (徐々に小さくする等)
//        if (p.scaleSpeed != 0.0f) {
//            float s = t.scale.x + p.scaleSpeed * dt;
//            if (s < 0.0f) s = 0.0f;
//            t.scale = { s, s, s };
//        }
//
//        // 4. 回転 (適当に回す)
//        t.rotation.x += 5.0f * dt;
//        t.rotation.y += 3.0f * dt;
//    }
//}