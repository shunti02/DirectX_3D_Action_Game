#pragma once
#include "ECS/System.h"

class Registry;

class PlayerSystem : public System {
public:
    bool tabKeyPressed = false;

    void Update(float dt) override;

private:
    void SwitchCharacter(Registry* registry);
};
