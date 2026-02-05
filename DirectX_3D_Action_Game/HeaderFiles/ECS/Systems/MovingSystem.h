#pragma once
#include "ECS/System.h"

class MovingSystem : public System {
public:
    void Update(float dt) override;
};
