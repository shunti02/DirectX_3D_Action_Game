#pragma once
#include <DirectXMath.h>

enum class PlayerType {
    AssaultStriker,
    BusterGuard,
    PlasmaSniper 
};

struct PlayerComponent {
   PlayerType type = PlayerType::AssaultStriker; 
    float maxHp = 100.0f;
    float currentHp = 100.0f;
    float totalAttack = 10.0f;
    float totalDefense = 5.0f;
    float totalWeight = 0.0f;

    float moveSpeed = 5.0f;
    float dashSpeed = 8.0f;
    float acceleration = 15.0f;
    float deceleration = 12.0f;
    float turnSpeed = 10.0f;
    float jumpPower = 5.0f;
    float gravity = 9.8f;

    DirectX::XMFLOAT3 velocity = { 0.0f, 0.0f, 0.0f };

    bool isGrounded = false;
    bool isDashing = false;
    bool isActive = true;

    float attackMultiplier = 1.0f;

    int currentActionType = 0;
    float actionTimer = 0.0f;
    bool isDead = false;
};