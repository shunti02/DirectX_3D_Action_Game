#include "ECS/Systems/CameraSystem.h"

#include "ECS/World.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/CameraComponent.h"
#include "App/Game.h"
#include <cmath>
#include <DirectXMath.h> // DirectXMathを明示的にインクルード

using namespace DirectX;

void CameraSystem::Update(float dt) {
    auto registry = pWorld->GetRegistry();
    Input* input = Game::GetInstance()->GetInput();

    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (!registry->HasComponent<CameraComponent>(id)) continue;
        if (!registry->HasComponent<TransformComponent>(id)) continue;

        auto& camera = registry->GetComponent<CameraComponent>(id);
        auto& cameraTrans = registry->GetComponent<TransformComponent>(id);

        // ★追従対象がいる場合の処理
        if (camera.targetEntityID != ECSConfig::INVALID_ID &&
            registry->HasComponent<TransformComponent>(camera.targetEntityID))
        {
            // ターゲット（プレイヤー）のTransformを取得
            auto& targetTrans = registry->GetComponent<TransformComponent>(camera.targetEntityID);

            // ターゲットの位置
            XMVECTOR targetPos = XMVectorSet(targetTrans.position.x, targetTrans.position.y, targetTrans.position.z, 0.0f);

            // 注視点（ターゲットの座標 + 腰へのオフセット）
            XMVECTOR focus = targetPos + XMVectorSet(0.0f, camera.lookAtOffset, 0.0f, 0.0f);

            // カメラの位置計算
            // ターゲットのY軸回転（ラジアン）
            float angleY = targetTrans.rotation.y;

            // 背後へのベクトル計算 (Z+が前とした場合)
            float eyeX = targetTrans.position.x - sinf(angleY) * camera.distance;
            float eyeZ = targetTrans.position.z - cosf(angleY) * camera.distance;
            float eyeY = targetTrans.position.y + camera.height;

            XMVECTOR eye = XMVectorSet(eyeX, eyeY, eyeZ, 0.0f);
            XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

            // ビュー行列更新
            camera.view = XMMatrixLookAtLH(eye, focus, up);

            // TransformComponent側も同期させておく
            cameraTrans.position = { eyeX, eyeY, eyeZ };
        }
        // ★追従対象がいない場合（デバッグ移動）
        else
        {
            float camSpeed = 10.0f * dt;
            if (input->IsKey(VK_UP))    cameraTrans.position.z += camSpeed;
            if (input->IsKey(VK_DOWN))  cameraTrans.position.z -= camSpeed;
            if (input->IsKey(VK_LEFT))  cameraTrans.position.x -= camSpeed;
            if (input->IsKey(VK_RIGHT)) cameraTrans.position.x += camSpeed;

            XMVECTOR eye = XMVectorSet(cameraTrans.position.x, cameraTrans.position.y, cameraTrans.position.z, 0.0f);
            XMVECTOR focus = XMVectorSet(cameraTrans.position.x, cameraTrans.position.y, cameraTrans.position.z + 1.0f, 0.0f);
            XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            camera.view = XMMatrixLookAtLH(eye, focus, up);
        }

        // プロジェクション行列（共通）
        camera.projection = XMMatrixPerspectiveFovLH(
            camera.fov, camera.aspectRatio, camera.nearZ, camera.farZ
        );
    }
}