/*===================================================================
//ファイル:PlayerComponent.h
//概要:プレイヤーの移動パラメータと状態
=====================================================================*/
#pragma once
#include <DirectXMath.h>

struct PlayerComponent {
    float moveSpeed;            // 歩く速さ
    float jumpPower;            // ジャンプ力
    float gravity;              // 重力の強さ

    DirectX::XMFLOAT3 velocity; // 現在の速度 (X, Y, Z)
    bool isGrounded;            // 地面に足がついているか？

    //このキャラが操作可能かどうか
    bool isActive;

    PlayerComponent(float speed = 5.0f)
        : moveSpeed(speed), jumpPower(8.0f), gravity(25.0f),
        velocity(0, 0, 0), isGrounded(false), isActive(false) {}
};