#pragma once

struct RecoverySphereComponent {
    float radius;
    int healAmount;
    bool isActive;

    int capacity;

    float rotationAngle = 0.0f;
};