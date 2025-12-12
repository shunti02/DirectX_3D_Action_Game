/*===================================================================
//ファイル:ColliderComponent.h
//概要:物理衝突判定用のデータを持つコンポーネント
=====================================================================*/
#pragma once
#include <DirectXMath.h>

enum class ColliderType {
    Type_Box,       // 立方体 (AABB)
    Type_Capsule,   // カプセル
    Type_Sphere     // 球
};

struct ColliderComponent {
    ColliderType type;

    // 共通データ
    DirectX::XMFLOAT3 center; // オフセット（モデル中心からのズレ）

    // Box用データ
    DirectX::XMFLOAT3 size;   // 幅・高さ・奥行き

    // Capsule/Sphere用データ
    float radius;
    float height; // Capsuleのみ使用

    // コンストラクタ（デフォルトはBOX）
    ColliderComponent()
        : type(ColliderType::Type_Box), center(0, 0, 0), size(1, 1, 1), radius(0.5f), height(1.0f) {}

    // 初期化用ヘルパー
    void SetBox(float width, float h, float depth) {
        type = ColliderType::Type_Box;
        size = { width, h, depth };
    }

    void SetCapsule(float r, float h) {
        type = ColliderType::Type_Capsule;
        radius = r;
        height = h;
    }
};
