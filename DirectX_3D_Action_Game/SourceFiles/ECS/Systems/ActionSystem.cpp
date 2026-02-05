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
#include "ECS/Components/BulletComponent.h"
#include "Game/EntityFactory.h"
#include "App/Game.h"
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
    // 攻撃球の更新 (広げる＆寿命)
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

    // ---------------------------------------------------------
    // 2. ★修正: 設置型回復スポット (エネルギータンク方式)
    // ---------------------------------------------------------
    EntityID playerID = ECSConfig::INVALID_ID;
    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (registry->HasComponent<PlayerComponent>(id)) {
            if (registry->GetComponent<PlayerComponent>(id).isActive) {
                playerID = id;
                break;
            }
        }
    }

    // プレイヤーがいて、かつ回復スポットがある場合
    if (playerID != ECSConfig::INVALID_ID) {
        auto& pTrans = registry->GetComponent<TransformComponent>(playerID);

        // 全エンティティから回復スポットを探す
        for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
            if (!registry->HasComponent<RecoverySphereComponent>(id)) continue;

            auto& sphere = registry->GetComponent<RecoverySphereComponent>(id);
            if (!registry->HasComponent<TransformComponent>(id)) continue;
            auto& sTrans = registry->GetComponent<TransformComponent>(id);

            // 無効または空なら処理しない
            if (!sphere.isActive || sphere.capacity <= 0) {
                // 見た目を消す (スケール0)
                sTrans.scale = { 0.0f, 0.0f, 0.0f };
                continue;
            }

            // 演出: クルクル回す
            sphere.rotationAngle += 2.0f * dt;
            sTrans.rotation.y = sphere.rotationAngle;

            // 距離判定 (XZ平面)
            float dx = pTrans.position.x - sTrans.position.x;
            float dz = pTrans.position.z - sTrans.position.z;
            float distSq = dx * dx + dz * dz;
            float hitR = 1.0f + sphere.radius;

            if (distSq < hitR * hitR) {
                // 範囲内にいる
                if (registry->HasComponent<StatusComponent>(playerID)) {
                    auto& status = registry->GetComponent<StatusComponent>(playerID);

                    // 「HPが減っている」かつ「タンクに残量がある」なら回復
                    if (status.hp < status.maxHp && sphere.capacity > 0) {

                        // 1フレームあたりの回復量 (※dt依存にせず固定値で少しずつ回復)
                        int healRate = 1;

                        // 残量チェック
                        if (sphere.capacity < healRate) healRate = sphere.capacity;

                        // 回復実行
                        status.hp += healRate;
                        if (status.hp > status.maxHp) status.hp = status.maxHp;

                        // タンク消費
                        sphere.capacity -= healRate;

                        // 回復エフェクト音 (連続再生しすぎないように制御が必要だが簡易的に)
                        // if (Game::GetInstance()->GetAudio()) Game::GetInstance()->GetAudio()->Play("SE_HEAL");
                    }
                }
            }

            // 使い切ったら非表示
            if (sphere.capacity <= 0) {
                sphere.isActive = false;
                AppLog::AddLog("Energy Cell Depleted.");
            }
        }
    }

    // ---------------------------------------------------------
     // 3. プレイヤー入力 (攻撃)
     // ---------------------------------------------------------
    if (playerID != ECSConfig::INVALID_ID) {
        if (registry->HasComponent<PlayerComponent>(playerID) &&
            registry->HasComponent<ActionComponent>(playerID))
        {
            auto& player = registry->GetComponent<PlayerComponent>(playerID);
            auto& action = registry->GetComponent<ActionComponent>(playerID);
            auto& trans = registry->GetComponent<TransformComponent>(playerID);

            if (player.isActive) {
                // クールタイム減少
                if (action.cooldownTimer > 0.0f) {
                    action.cooldownTimer -= dt;
                }
                else {
                    // クールダウンが終わったら攻撃モーションフラグを下ろす
                    action.isAttacking = false;
                }

                // 左クリックで攻撃
                if (input->IsMouseKeyDown(0) && action.cooldownTimer <= 0.0f) {
                    action.isAttacking = true;
                    action.cooldownTimer = action.attackCooldown;

                    // 攻撃発生位置
                    float angle = trans.rotation.y;
                    float dist = 2.0f;
                    XMFLOAT3 spawnPos = {
                        trans.position.x + sinf(angle) * dist,
                        trans.position.y + 0.5f,
                        trans.position.z + cosf(angle) * dist
                    };

                    int dmg = 10;
                    if (registry->HasComponent<StatusComponent>(playerID)) {
                        dmg = registry->GetComponent<StatusComponent>(playerID).attackPower;
                    }

                    // ----------------------------------------------------------------
                     // ★修正: プレイヤータイプに応じて攻撃を変える
                     // ----------------------------------------------------------------
                    if (player.type == PlayerType::PlasmaSniper) {
                        // === Type C: 弾を発射 (Plasma Bullet) ===

                        // カメラの向きを取得（弾を飛ばす方向）
                        XMVECTOR camDir = XMVectorSet(0, 0, 1, 0); // デフォルト前方

                        // カメラエンティティを探して向きを取得
                        for (EntityID camID = 0; camID < ECSConfig::MAX_ENTITIES; ++camID) {
                            if (registry->HasComponent<CameraComponent>(camID)) {
                                auto& cam = registry->GetComponent<CameraComponent>(camID);

                                // ★修正: 上下(angleX)と左右(angleY)の両方を使って向きを決める
                                // 行列を使って「真ん前(0,0,1)」をカメラの角度分だけ回転させる
                                XMMATRIX camRot = XMMatrixRotationRollPitchYaw(cam.angleX, cam.angleY, 0.0f);
                                camDir = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), camRot);
                                break;
                            }
                        }

                        XMFLOAT3 spawnPos = trans.position;
                        spawnPos.y += 1.0f; // 胸の高さ

                        // 少し前に出す
                        XMVECTOR posV = XMLoadFloat3(&spawnPos);
                        posV += camDir * 1.5f;
                        XMStoreFloat3(&spawnPos, posV);

                        XMFLOAT3 dirF; XMStoreFloat3(&dirF, camDir);

                        // ★追加: マズルフラッシュ発生
                        EntityFactory::CreateMuzzleFlash(pWorld, spawnPos, dirF);

                        // 弾生成
                        EntityFactory::CreatePlayerBullet(pWorld, spawnPos, dirF, dmg);

                        // SE
                        if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_SWITCH");
                    }
                    else {
                        // === Type A & B: 範囲攻撃 (Sphere) ===

                        // 攻撃発生位置
                        float angle = trans.rotation.y;
                        float dist = 2.0f;
                        XMFLOAT3 spawnPos = {
                            trans.position.x + sinf(angle) * dist,
                            trans.position.y + 0.5f,
                            trans.position.z + cosf(angle) * dist
                        };

                        // 攻撃球生成
                        EntityFactory::CreateAttackSphere(pWorld, playerID, spawnPos, dmg);

                        // SE
                        if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_JUMP");
                    }
                }
            }
        }
    }

    // ---------------------------------------------------------
     // 4.敵の弾 (Bullet) の更新
     // ---------------------------------------------------------
     // プレイヤーの位置取得などは不要になったので削除してOK

     // 弾のループ
    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (!registry->HasComponent<BulletComponent>(id)) continue;

        auto& bullet = registry->GetComponent<BulletComponent>(id);

        // 1. 寿命管理のみ行う
        bullet.lifeTime -= dt;
        if (bullet.lifeTime <= 0.0f) {
            pWorld->DestroyEntity(id);
            continue;
        }
    }
}