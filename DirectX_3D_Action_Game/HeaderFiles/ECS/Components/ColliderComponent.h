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
    ColliderType type = ColliderType::Type_Box;

    // 共通データ
    DirectX::XMFLOAT3 center = { 0.0f, 0.0f, 0.0f }; // オフセット

    // Box用データ
    DirectX::XMFLOAT3 size = { 1.0f, 1.0f, 1.0f };   // 幅・高さ・奥行き

    // Capsule/Sphere用データ
    float radius = 0.5f;
    float height = 1.0f; // Capsuleのみ使用

    // ★コンストラクタは削除しました

    // 初期化用ヘルパー関数はあってもOKです
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
