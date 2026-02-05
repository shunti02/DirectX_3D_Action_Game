/*===================================================================
// ファイル: PlayerComponent.h
// 概要: プレイヤーのパラメータとタイプ定義（宇宙編）
=====================================================================*/
#pragma once
#include <DirectXMath.h>

// プレイヤーの武装タイプ
enum class PlayerType {
    AssaultStriker, // バランス（剣）
    BusterGuard,    // パワー（ハンマー）
    PlasmaSniper    // 遠距離（銃）
};

struct PlayerComponent {
    PlayerType type = PlayerType::AssaultStriker; // デフォルト

    float moveSpeed = 5.0f;
    float jumpPower = 5.0f;
    float gravity = 9.8f;

    DirectX::XMFLOAT3 velocity = { 0.0f, 0.0f, 0.0f };

    bool isGrounded = false;
    bool isActive = true; // 基本常にTrue（ソロプレイのため）

    // 攻撃力倍率などをここに追加しても良い
    float attackMultiplier = 1.0f;
};