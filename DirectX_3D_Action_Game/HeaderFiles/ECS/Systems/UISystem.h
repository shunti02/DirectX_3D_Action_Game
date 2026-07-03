#pragma once
#include "ECS/System.h"

class Graphics;

class UISystem : public System {
public:
	void Init(World* world) override;
	void Update(float dt) override;
	void Draw(Graphics* pGraphics);
};