/*===================================================================
//ファイル:ActionSystem.cpp
//概要:攻撃入力の検知と攻撃判定の生成（実装）
=====================================================================*/
#include "ECS/Systems/ActionSystem.h"

//コンポーネント群
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/ActionComponent.h"
#include "ECS/Components/RolesComponent.h"
#include "ECS/Components/AttackBoxComponent.h"
#include "ECS/Components/RecoveryBoxComponent.h"
#include "ECS/Components/AttackSphereComponent.h"
#include "ECS/Components/RecoverySphereComponent.h"
#include "ECS/Components/StatusComponent.h"

#include "Game/EntityFactory.h"
//#include "App/Game.h"
#include <DirectXMath.h>

using namespace DirectX;

void ActionSystem::Update(float dt) {
    auto registry = pWorld->GetRegistry();
    Input* input = Game::GetInstance()->GetInput();

    //攻撃ボックスの寿命管理
    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (registry->HasComponent<AttackBoxComponent>(id)) {
            auto& box = registry->GetComponent<AttackBoxComponent>(id);
            box.lifeTime -= dt;
            if (box.lifeTime <= 0.0f) {
                pWorld->DestroyEntity(id);//寿命が尽きたら消す
            }
        }
    }
    //回復ボックスの寿命管理
    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (registry->HasComponent<RecoveryBoxComponent>(id)) {
            auto& box = registry->GetComponent<RecoveryBoxComponent>(id);
            box.lifeTime -= dt;
            if (box.lifeTime <= 0.0f) pWorld->DestroyEntity(id);
        }
    }
    // ★追加: 攻撃球の更新 (広げる＆寿命)
    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (registry->HasComponent<AttackSphereComponent>(id)) {
            auto& sphere = registry->GetComponent<AttackSphereComponent>(id);
            sphere.lifeTime -= dt;
            sphere.currentRadius += sphere.expansionSpeed * dt;
            if (sphere.currentRadius > sphere.maxRadius) sphere.currentRadius = sphere.maxRadius;
            //Transformのスケールを現在の半径に合わせる
            if (registry->HasComponent<TransformComponent>(id)) {
                auto& trans = registry->GetComponent<TransformComponent>(id);
                // 半径 r の球を表示するには、直径 2r 倍のスケールが必要な場合と、半径そのままの場合があります。
                // 通常の単位球(半径1)ならスケールは r でOKです。
                // ここでは GeometryGenerator の球が半径1と仮定して、スケールを半径に合わせます。
                trans.scale = { sphere.currentRadius, sphere.currentRadius, sphere.currentRadius };
            }
            if (sphere.lifeTime <= 0.0f) pWorld->DestroyEntity(id);
        }
    }

    // ★追加: 回復球の更新
    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (registry->HasComponent<RecoverySphereComponent>(id)) {
            auto& sphere = registry->GetComponent<RecoverySphereComponent>(id);
            sphere.lifeTime -= dt;
            sphere.currentRadius += sphere.expansionSpeed * dt;
            if (sphere.currentRadius > sphere.maxRadius) sphere.currentRadius = sphere.maxRadius;
            // ★追加: スケール同期
            if (registry->HasComponent<TransformComponent>(id)) {
                auto& trans = registry->GetComponent<TransformComponent>(id);
                trans.scale = { sphere.currentRadius, sphere.currentRadius, sphere.currentRadius };
            }
            if (sphere.lifeTime <= 0.0f) pWorld->DestroyEntity(id);
        }
    }

    //プレイヤーの入力処理
    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        //必要なエンティティを持っているか
        if (!registry->HasComponent<PlayerComponent>(id)) continue;
        if (!registry->HasComponent<ActionComponent>(id)) continue;

        auto& player = registry->GetComponent<PlayerComponent>(id);
        auto& action = registry->GetComponent<ActionComponent>(id);
        auto& trans = registry->GetComponent<TransformComponent>(id);

        //走査中のキャラでなければスキップ
        if (!player.isActive) continue;

        //クールタイムを減らす
        if (action.cooldownTimer > 0.0f) {
            action.cooldownTimer -= dt;
        }

        //左クリックで攻撃
        if (input->IsMouseKeyDown(0) && action.cooldownTimer <= 0.0f) {
            //攻撃開始
            action.isAttacking = true;
            action.cooldownTimer = action.attackCooldown;

            //攻撃判定の位置計算
            //プレイヤーの向きに合わせて前方にオフセット
            float angle = trans.rotation.y;
            float dist = 3.0f;
            //DirectX座標系：Zが奥、Xが右
            //回転が0のときZ+を向いているなら
            float offsetX = sinf(angle) * dist;
            float offsetZ = cosf(angle) * dist;

            XMFLOAT3 spawnPos = {
                trans.position.x + offsetX,
                trans.position.y + 0.5f,
                trans.position.z + offsetZ
            };

            //攻撃判定サイズ
            XMFLOAT3 size = { 3.0f, 3.0f, 3.0f };

            // ★役割による分岐
            if (registry->HasComponent<AttackerTag>(id)) {
                // アタッカーなら攻撃
                int dmg = 10;
                // StatusComponentのチェックを追加（元コードに参照があったため）
                if (registry->HasComponent<StatusComponent>(id)) {
                    dmg = registry->GetComponent<StatusComponent>(id).attackPower;
                }
               // EntityFactory::CreateAttackHitbox(pWorld, id, spawnPos, size, dmg);
                EntityFactory::CreateAttackSphere(pWorld, id, spawnPos, dmg);
            }
            else if (registry->HasComponent<HealerTag>(id)) {
                // ヒーラーなら回復 (回復判定は少し大きめに 2.5倍)
                int heal = 10; // 固定回復量
                XMFLOAT3 healSize = { 5.0f, 5.0f, 5.0f };
                //EntityFactory::CreateRecoveryHitbox(pWorld, id, spawnPos, healSize, heal);
                EntityFactory::CreateRecoverySphere(pWorld, id, trans.position, heal);
            }
        }
    }
}