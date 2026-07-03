#pragma once
#include "ECS/System.h"

class CameraSystem : public System {
public:
    void Update(float dt) override;
};