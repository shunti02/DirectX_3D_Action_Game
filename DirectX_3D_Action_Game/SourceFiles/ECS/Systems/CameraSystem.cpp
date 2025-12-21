#include "ECS/Systems/CameraSystem.h"

#include "ECS/World.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/CameraComponent.h"
#include "App/Game.h"
#include <cmath>
#include <algorithm>
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
            // 1. カメラの回転操作 (矢印キー)
            float rotSpeed = 2.0f * dt;

            // ←→キーで周囲を回転 (Y軸回転)
            if (input->IsKey(VK_RIGHT)) camera.angleY += rotSpeed;
            if (input->IsKey(VK_LEFT))  camera.angleY -= rotSpeed;

            // ↑↓キーで高さを調整 (X軸回転)
            if (input->IsKey(VK_UP))    camera.angleX -= rotSpeed;
            if (input->IsKey(VK_DOWN))  camera.angleX += rotSpeed;

            // 角度制限 (真上・真下に行き過ぎないように -80度～80度くらいで止める)
            // 1.4 rad ≒ 80度
            camera.angleX = std::clamp(camera.angleX, -1.4f, 1.4f);


            // 2. ターゲット情報の取得
            auto& targetTrans = registry->GetComponent<TransformComponent>(camera.targetEntityID);
            XMVECTOR targetPos = XMVectorSet(targetTrans.position.x, targetTrans.position.y, targetTrans.position.z, 0.0f);

            // 注視点 (プレイヤーの少し上)
            XMVECTOR focus = targetPos + XMVectorSet(0.0f, camera.lookAtOffset, 0.0f, 0.0f);


            // 3. カメラ位置の計算 (球座標系)
            // プレイヤーの向きではなく、カメラ自身の angleX, angleY を使う

            // 水平距離 (高さによる補正)
            float hDist = camera.distance * cosf(camera.angleX);
            // 垂直高さ
            float vDist = camera.distance * sinf(camera.angleX);

            // ターゲットから見たカメラの位置オフセット
            float offsetX = -sinf(camera.angleY) * hDist;
            float offsetZ = -cosf(camera.angleY) * hDist;
            float offsetY = vDist;

            // 最終的なカメラ位置 = 注視点 + オフセット
            // (注視点を中心に回るため、focusに足します)
            XMVECTOR eye = focus + XMVectorSet(offsetX, offsetY, offsetZ, 0.0f);
            XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

            // 4. 行列更新
            camera.view = XMMatrixLookAtLH(eye, focus, up);

            // Transformにも反映 (レンダリング以外の用途があれば使う)
            XMFLOAT3 eyePos;
            XMStoreFloat3(&eyePos, eye);
            cameraTrans.position = eyePos;
            //// ターゲット（プレイヤー）のTransformを取得
            //auto& targetTrans = registry->GetComponent<TransformComponent>(camera.targetEntityID);

            //// ターゲットの位置
            //XMVECTOR targetPos = XMVectorSet(targetTrans.position.x, targetTrans.position.y, targetTrans.position.z, 0.0f);

            //// 注視点（ターゲットの座標 + 腰へのオフセット）
            //XMVECTOR focus = targetPos + XMVectorSet(0.0f, camera.lookAtOffset, 0.0f, 0.0f);

            //// カメラの位置計算
            //// ターゲットのY軸回転（ラジアン）
            //float angleY = targetTrans.rotation.y;

            //// 背後へのベクトル計算 (Z+が前とした場合)
            //float eyeX = targetTrans.position.x - sinf(angleY) * camera.distance;
            //float eyeZ = targetTrans.position.z - cosf(angleY) * camera.distance;
            //float eyeY = targetTrans.position.y + camera.height;

            //XMVECTOR eye = XMVectorSet(eyeX, eyeY, eyeZ, 0.0f);
            //XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

            //// ビュー行列更新
            //camera.view = XMMatrixLookAtLH(eye, focus, up);

            //// TransformComponent側も同期させておく
            //cameraTrans.position = { eyeX, eyeY, eyeZ };
        }
        // ★追従対象がいない場合（デバッグ移動）
        else
        {
            float camSpeed = 10.0f * dt;
            if (input->IsKey('I'))    cameraTrans.position.z += camSpeed; // I/K/J/Lに変更例
            if (input->IsKey('K'))  cameraTrans.position.z -= camSpeed;
            if (input->IsKey('J'))  cameraTrans.position.x -= camSpeed;
            if (input->IsKey('L')) cameraTrans.position.x += camSpeed;

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