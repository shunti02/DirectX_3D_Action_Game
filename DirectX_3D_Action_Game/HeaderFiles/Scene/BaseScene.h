#pragma once
#include <memory>
#include "ECS/World.h"

class SceneManager;

class BaseScene {
public:
    BaseScene(SceneManager* manager) : pManager(manager) {
        pWorld = std::make_unique<World>();
    }
    virtual ~BaseScene() = default;
    virtual void Initialize() = 0;

    virtual void Update(float dt) {
        if (pWorld) pWorld->Update(dt);
    }

    virtual void Draw() {
        if (pWorld) pWorld->Draw();
    }

    virtual void Shutdown() {
    }

protected:
    SceneManager* pManager;
    std::unique_ptr<World> pWorld;
};
