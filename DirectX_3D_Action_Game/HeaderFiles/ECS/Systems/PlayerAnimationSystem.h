#pragma once
#include "ECS/System.h"

class PlayerAnimationSystem : public System {
public:
    PlayerAnimationSystem(){}
    void Update(float dt) override;
    void Draw() override {}
private:
    float timeAccumulator = 0.0f;
};
