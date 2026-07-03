#pragma once
#include <DirectXMath.h>
#include "ECS/Components/PlayerPartComponent.h"

struct EnemyPartComponent {
    int parentID = -1;
    PartType partType;
    int partModelID = 0;
    DirectX::XMFLOAT3 baseOffset = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 baseRotation = { 0.0f, 0.0f, 0.0f };
};
