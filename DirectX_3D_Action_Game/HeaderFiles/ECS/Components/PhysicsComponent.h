#pragma once
#include <DirectXMath.h>

struct PhysicsComponent {
    DirectX::XMFLOAT3 velocity = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 acceleration = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 force = { 0.0f, 0.0f, 0.0f };
    float mass = 1.0f;
    float drag = 0.0f;     // ‹ó‹C’ïR
    bool useGravity = true; // d—Í‚ğó‚¯‚é‚©
    bool isGrounded = false;
};
