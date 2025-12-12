#pragma once
#include <DirectXMath.h>
#include "ECS/Component.h"

struct CameraComponent {
    DirectX::XMMATRIX view;       // ビュー行列（カメラの位置・向き）
    DirectX::XMMATRIX projection; // プロジェクション行列（遠近法）

    float fov;         // 画角 (Field of View)
    float aspectRatio; // アスペクト比
    float nearZ;       // これより手前は描画しない
    float farZ;        // これより奥は描画しない

    EntityID targetEntityID;//対象
    float distance;//対象との距離
    float height;//カメラの高さ
    float lookAtOffset;//注視点の高さ

    CameraComponent()
        : fov(DirectX::XM_PIDIV4), aspectRatio(16.0f / 9.0f), nearZ(0.1f), farZ(1000.0f),
        targetEntityID(ECSConfig::INVALID_ID), // 初期値は追従なし
        distance(5.0f), height(3.0f), lookAtOffset(1.0f) // デフォルト値
    {
        view = DirectX::XMMatrixIdentity();
        projection = DirectX::XMMatrixIdentity();
    }
};
