#pragma once

class World;

class System {
public:
	virtual ~System() = default;

	virtual void Init(World* world) { pWorld = world; }

	virtual void Update(float dt){}

	virtual void Draw(){}

protected:
	World* pWorld = nullptr;
};
