#pragma once
#include <DirectXMath.h>
#include "ECS/Component.h"

struct CameraComponent {
    DirectX::XMMATRIX view = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX projection = DirectX::XMMatrixIdentity();

    float fov = DirectX::XM_PIDIV4;
    float aspectRatio = 16.0f / 9.0f;
    float nearZ = 0.1f;
    float farZ = 1000.0f;

    EntityID targetEntityID = ECSConfig::INVALID_ID;
    float distance = 10.0f;
    float height = 3.0f;
    float lookAtOffset = 1.0f;

    float angleX = 0.2f;
    float angleY = 0.0f;

};
