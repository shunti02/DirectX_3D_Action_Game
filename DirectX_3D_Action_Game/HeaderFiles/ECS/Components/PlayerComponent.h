/*===================================================================
//ファイル:PlayerComponent.h
//概要:プレイヤーの移動パラメータと状態
=====================================================================*/
#pragma once
#include <DirectXMath.h>

struct PlayerComponent {
    float moveSpeed = 5.0f;
    float jumpPower = 5.0f;
    float gravity = 9.8f;
    DirectX::XMFLOAT3 velocity = { 0.0f, 0.0f, 0.0f };
    bool isGrounded = false;
    bool isActive = false;
};