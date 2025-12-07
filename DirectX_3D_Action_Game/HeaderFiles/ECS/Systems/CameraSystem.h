#pragma once
#include "ECS/System.h"
#include "ECS/World.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/CameraComponent.h"
#include "App/Game.h"

using namespace DirectX;

class CameraSystem : public System {
public:
    void Update(float dt) override {
        auto registry = pWorld->GetRegistry();
        Input* input = Game::GetInstance()->GetInput();

        // 全てのカメラEntityを更新（通常はメインカメラ1つ）
        for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
            if (!registry->HasComponent<CameraComponent>(id)) continue;
            if (!registry->HasComponent<TransformComponent>(id)) continue;

            auto& camera = registry->GetComponent<CameraComponent>(id);
            auto& trans = registry->GetComponent<TransformComponent>(id);

            //矢印キーでのカメラ移動 (デバッグ用)
            float camSpeed = 10.0f * dt;
            if (input->IsKey(VK_UP))    trans.position.z += camSpeed;
            if (input->IsKey(VK_DOWN))  trans.position.z -= camSpeed;
            if (input->IsKey(VK_LEFT))  trans.position.x -= camSpeed;
            if (input->IsKey(VK_RIGHT)) trans.position.x += camSpeed;

            // 1. ビュー行列の計算 (カメラの位置と回転から)
            // 簡易的に、回転はまだ考慮せず「位置」だけ反映させます
            // 本格的なFPSカメラなどはここに回転計算を追加します
            XMVECTOR eye = XMVectorSet(trans.position.x, trans.position.y, trans.position.z, 0.0f);
            XMVECTOR focus = XMVectorSet(trans.position.x, trans.position.y, trans.position.z + 1.0f, 0.0f); // 原点を見る
            XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);    // 上方向

            camera.view = XMMatrixLookAtLH(eye, focus, up);

            // 2. プロジェクション行列の計算 (遠近法の設定)
            camera.projection = XMMatrixPerspectiveFovLH(
                camera.fov, camera.aspectRatio, camera.nearZ, camera.farZ
            );
        }
    }
};
