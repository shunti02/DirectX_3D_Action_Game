#pragma once

struct RecoverySphereComponent {
    int ownerID = -1;
    int healAmount = 0;
    float lifeTime = 0.5f;

    float currentRadius = 1.0f;
    float maxRadius = 5.0f;
    float expansionSpeed = 15.0f;
};