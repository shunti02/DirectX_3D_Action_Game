#pragma once
#include <DirectXMath.h>

struct MovingComponent {
    DirectX::XMFLOAT3 startPos;
    DirectX::XMFLOAT3 moveVec; // ˆÚ“®—Ê (U•)
    float speed = 1.0f;
    float time = 0.0f;
};
