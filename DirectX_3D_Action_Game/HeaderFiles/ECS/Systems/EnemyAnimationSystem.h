#pragma once
#include "ECS/System.h"

class EnemyAnimationSystem : public System {
public:
    void Update(float dt) override;
private:
    float timeAccumulator = 0.0f;
};
