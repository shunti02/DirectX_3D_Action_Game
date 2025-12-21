#pragma once
#include <DirectXMath.h>
#include "ECS/Component.h"

struct CameraComponent {
    DirectX::XMMATRIX view = DirectX::XMMatrixIdentity();       // ビュー行列
    DirectX::XMMATRIX projection = DirectX::XMMatrixIdentity(); // プロジェクション行列

    float fov = DirectX::XM_PIDIV4; // 画角 (45度)
    float aspectRatio = 16.0f / 9.0f;
    float nearZ = 0.1f;
    float farZ = 1000.0f;

    EntityID targetEntityID = ECSConfig::INVALID_ID; // 追従対象なし
    float distance = 5.0f;     // 対象との距離
    float height = 3.0f;       // カメラの高さ
    float lookAtOffset = 1.0f; // 注視点の高さ

    // ★追加: カメラ自体の回転角度 (プレイヤーの向きとは独立させるため)
    float angleX = 0.2f; // 上下角度 (ピッチ) - 初期値で少し見下ろす
    float angleY = 0.0f; // 左右角度 (ヨー)
};
