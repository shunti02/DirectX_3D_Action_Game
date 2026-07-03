#pragma once
#include <DirectXMath.h>

enum class ColliderType {
    Type_None,
    Type_Box,
    Type_Capsule,
    Type_Sphere
};

struct ColliderComponent {
    ColliderType type = ColliderType::Type_Box;

    DirectX::XMFLOAT3 center = { 0.0f, 0.0f, 0.0f }; 
    DirectX::XMFLOAT3 size = { 1.0f, 1.0f, 1.0f };
    float radius = 0.5f;
    float height = 1.0f; 
    void SetBox(float width, float h, float depth) {
        type = ColliderType::Type_Box;
        size = { width, h, depth };
    }

    void SetCapsule(float r, float h) {
        type = ColliderType::Type_Capsule;
        radius = r;
        height = h;
    }

    void SetSphere(float r) {
        type = ColliderType::Type_Sphere;
        radius = r;
    }
};
