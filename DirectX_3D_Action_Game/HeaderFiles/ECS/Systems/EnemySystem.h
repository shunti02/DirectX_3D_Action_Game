#pragma once
#include "ECS/System.h"

class EnemySystem : public System {
public:
    void Update(float dt) override;
    // š’Ç‰Á: Œo‰ßŠÔ‚ğŒv‘ª‚·‚é•Ï”
    float timeAccumulator = 0.0f;
};