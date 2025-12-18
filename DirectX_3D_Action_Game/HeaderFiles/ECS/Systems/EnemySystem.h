#pragma once
#include "ECS/System.h"

class EnemySystem : public System {
public:
    void Update(float dt) override;
};