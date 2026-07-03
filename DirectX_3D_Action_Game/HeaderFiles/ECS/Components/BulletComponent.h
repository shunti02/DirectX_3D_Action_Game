#pragma once

struct BulletComponent {
    int damage;
    float lifeTime;
    bool isActive;
    bool fromPlayer = false;
};
