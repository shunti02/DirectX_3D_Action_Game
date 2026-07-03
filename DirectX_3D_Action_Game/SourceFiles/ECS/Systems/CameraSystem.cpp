#include "ECS/Systems/CameraSystem.h"

#include "ECS/World.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/CameraComponent.h"
#include "App/Game.h"
#include <cmath>
#include <algorithm>
#include <DirectXMath.h>
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

        if (camera.targetEntityID != ECSConfig::INVALID_ID &&
            registry->HasComponent<TransformComponent>(camera.targetEntityID))
        {
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

            float rotSpeed = 2.0f * dt;

            if (input->IsKey(VK_RIGHT)) camera.angleY += rotSpeed;
            if (input->IsKey(VK_LEFT))  camera.angleY -= rotSpeed;

            if (input->IsKey(VK_UP))    camera.angleX -= rotSpeed;
            if (input->IsKey(VK_DOWN))  camera.angleX += rotSpeed;

            camera.angleX = std::clamp(camera.angleX, -1.4f, 1.4f);

            auto& targetTrans = registry->GetComponent<TransformComponent>(camera.targetEntityID);
            XMVECTOR targetPos = XMVectorSet(targetTrans.position.x, targetTrans.position.y, targetTrans.position.z, 0.0f);

            XMVECTOR focus = targetPos + XMVectorSet(0.0f, camera.lookAtOffset, 0.0f, 0.0f);

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
        else
        {
            float camSpeed = 10.0f * dt;
            if (input->IsKey('I'))    cameraTrans.position.z += camSpeed;
            if (input->IsKey('K'))  cameraTrans.position.z -= camSpeed;
            if (input->IsKey('J'))  cameraTrans.position.x -= camSpeed;
            if (input->IsKey('L')) cameraTrans.position.x += camSpeed;

            XMVECTOR eye = XMVectorSet(cameraTrans.position.x, cameraTrans.position.y, cameraTrans.position.z, 0.0f);
            XMVECTOR focus = XMVectorSet(cameraTrans.position.x, cameraTrans.position.y, cameraTrans.position.z + 1.0f, 0.0f);
            XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            camera.view = XMMatrixLookAtLH(eye, focus, up);
        }

        camera.projection = XMMatrixPerspectiveFovLH(
            camera.fov, camera.aspectRatio, camera.nearZ, camera.farZ
        );
    }
}