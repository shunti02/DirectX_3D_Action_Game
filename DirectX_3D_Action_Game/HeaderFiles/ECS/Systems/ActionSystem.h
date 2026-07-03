#pragma once

#include "ECS/System.h"
class ActionSystem : public System {
public:
    void Update(float dt) override;
};