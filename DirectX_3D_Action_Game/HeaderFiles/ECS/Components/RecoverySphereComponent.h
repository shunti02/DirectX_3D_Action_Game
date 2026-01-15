#pragma once

struct RecoverySphereComponent {
    float radius;
    int healAmount; // 1回あたりの回復量（または瞬時回復量）
    bool isActive;

    // ★追加: エネルギータンク方式用の容量
    int capacity;

    // 回転演出用
    float rotationAngle = 0.0f;
};