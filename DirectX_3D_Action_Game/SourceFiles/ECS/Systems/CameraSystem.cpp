#include "ECS/Systems/CameraSystem.h"

#include "ECS/World.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/CameraComponent.h"
#include "App/Game.h"
#include <cmath>
#include <algorithm>
#include <DirectXMath.h> // DirectXMathを明示的にインクルード
#include <Windows.h>
#include "App/Main.h"

using namespace DirectX;

void CameraSystem::Update(float dt) {
    auto registry = pWorld->GetRegistry();
    Input* input = Game::GetInstance()->GetInput();
    HWND hWnd = Game::GetInstance()->GetWindowHandle();
    for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
        if (!registry->HasComponent<CameraComponent>(id)) continue;
        if (!registry->HasComponent<TransformComponent>(id)) continue;

        auto& camera = registry->GetComponent<CameraComponent>(id);
        auto& cameraTrans = registry->GetComponent<TransformComponent>(id);

        // ★追従対象がいる場合の処理
        if (camera.targetEntityID != ECSConfig::INVALID_ID &&
            registry->HasComponent<TransformComponent>(camera.targetEntityID))
        {
            // ---------------------------------------------------------
            // 1. マウスによる回転操作
            // ---------------------------------------------------------
            if (GetForegroundWindow() == hWnd) {
                POINT center = { Config::SCREEN_WIDTH / 2, Config::SCREEN_HEIGHT / 2 };
                ClientToScreen(hWnd, &center);

                POINT mousePos;
                GetCursorPos(&mousePos);

                float deltaX = static_cast<float>(mousePos.x - center.x);
                float deltaY = static_cast<float>(mousePos.y - center.y);

                if (deltaX != 0.0f || deltaY != 0.0f) {
                    float sensitivity = 0.002f;
                    camera.angleY += deltaX * sensitivity;
                    camera.angleX += deltaY * sensitivity;

                    SetCursorPos(center.x, center.y);
                }
            }

            // ---------------------------------------------------------
            // 2. キーボードによる回転操作 (復活！)
            // ---------------------------------------------------------
            float rotSpeed = 2.0f * dt;

            // ←→キー (Y軸回転)
            if (input->IsKey(VK_RIGHT)) camera.angleY += rotSpeed;
            if (input->IsKey(VK_LEFT))  camera.angleY -= rotSpeed;

            // ↑↓キー (X軸回転)
            if (input->IsKey(VK_UP))    camera.angleX -= rotSpeed;
            if (input->IsKey(VK_DOWN))  camera.angleX += rotSpeed;

            // ---------------------------------------------------------
            // 3. 角度制限 (マウス・キー共通)
            // ---------------------------------------------------------
            // 真上・真下に行き過ぎないように制限
            camera.angleX = std::clamp(camera.angleX, -1.4f, 1.4f);

            // ---------------------------------------------------------
            // 4. ズーム操作
            // ---------------------------------------------------------
            float wheel = input->GetMouseWheel();
            if (wheel != 0.0f) {
                camera.distance -= wheel * 0.005f;
                camera.distance = std::clamp(camera.distance, 2.0f, 20.0f);
            }

            // ---------------------------------------------------------
            // 5. 行列計算 (追従処理)
            // ---------------------------------------------------------
            auto& targetTrans = registry->GetComponent<TransformComponent>(camera.targetEntityID);
            XMVECTOR targetPos = XMVectorSet(targetTrans.position.x, targetTrans.position.y, targetTrans.position.z, 0.0f);

            // 注視点
            XMVECTOR focus = targetPos + XMVectorSet(0.0f, camera.lookAtOffset, 0.0f, 0.0f);

            // カメラ位置 (球座標計算)
            float hDist = camera.distance * cosf(camera.angleX);
            float vDist = camera.distance * sinf(camera.angleX);

            float offsetX = -sinf(camera.angleY) * hDist;
            float offsetZ = -cosf(camera.angleY) * hDist;
            float offsetY = vDist;

            XMVECTOR eye = focus + XMVectorSet(offsetX, offsetY, offsetZ, 0.0f);
            XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

            camera.view = XMMatrixLookAtLH(eye, focus, up);

            XMFLOAT3 eyePos;
            XMStoreFloat3(&eyePos, eye);
            cameraTrans.position = eyePos;
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