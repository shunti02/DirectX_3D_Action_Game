#pragma once
#include "ECS/System.h"

class ParticleSystem : public System {
public:
    void Init(World* world) override;
    void Update(float dt) override;
};
