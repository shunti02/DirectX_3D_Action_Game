#pragma once
#include "ECS/System.h"

class EnemySystem : public System {
public:
    void Update(float dt) override;
    float timeAccumulator = 0.0f;
};