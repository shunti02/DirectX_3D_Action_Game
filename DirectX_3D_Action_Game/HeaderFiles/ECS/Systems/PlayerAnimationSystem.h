#pragma once
#include "ECS/System.h"

class PlayerAnimationSystem : public System {
public:
    PlayerAnimationSystem(){}
    void Update(float dt) override;
    void Draw() override {} // 描画はRenderSystemに任せる
private:
    float timeAccumulator = 0.0f; // アニメーション用タイマー
};
