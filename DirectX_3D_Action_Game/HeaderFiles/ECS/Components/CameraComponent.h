#pragma once
#include <DirectXMath.h>

struct CameraComponent {
    DirectX::XMMATRIX view;       // ビュー行列（カメラの位置・向き）
    DirectX::XMMATRIX projection; // プロジェクション行列（遠近法）

    float fov;         // 画角 (Field of View)
    float aspectRatio; // アスペクト比
    float nearZ;       // これより手前は描画しない
    float farZ;        // これより奥は描画しない

    CameraComponent()
        : fov(DirectX::XM_PIDIV4), aspectRatio(16.0f / 9.0f), nearZ(0.1f), farZ(1000.0f) {
        view = DirectX::XMMatrixIdentity();
        projection = DirectX::XMMatrixIdentity();
    }
};
