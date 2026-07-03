#pragma once

#include "ECS/ECS.h"
struct AttackBoxComponent {
        int ownerID = -1;
        int damage = 0;
        float lifeTime = 0.5f;
};
