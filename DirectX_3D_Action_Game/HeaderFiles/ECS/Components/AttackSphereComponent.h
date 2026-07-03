#pragma once

struct AttackSphereComponent {
    int ownerID = -1;
    int damage = 0;
    float lifeTime = 0.5f;

    float currentRadius = 0.5f;
    float maxRadius = 3.0f;
    float expansionSpeed = 10.0f;
};