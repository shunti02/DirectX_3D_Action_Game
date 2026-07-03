#pragma once
#include "ECS/ECS.h"
#include "ECS/System.h"
#include <vector>
#include <memory>

class World {
public:
	World() {
		registry = std::make_unique<Registry>();
	}
	~World() {
		for (auto* sys : systems)delete sys;
		systems.clear();
	}
	class EntityBuilder {
	public:
		EntityBuilder(World* world, EntityID id) : pWorld(world),entityID(id){}

		template <typename T, typename...Args>
		EntityBuilder& AddComponent(Args&&...args) {
			pWorld->registry->AddComponent<T>(entityID, std::forward<Args>(args)...);
			return *this;
		}
		EntityID Build() const { return entityID; }
	private:
		World* pWorld;
		EntityID entityID;
	};
	EntityBuilder CreateEntity() {
		EntityID id = registry->CreateEntity();
		return EntityBuilder(this, id);
	}
	template <typename T, typename...Args>
	void AddComponent(EntityID id, Args&&...args) {
		registry->AddComponent<T>(id, std::forward<Args>(args)...);
	}
	void DestroyEntity(EntityID id) {
		registry->DestroyEntity(id);
	}
	template <typename T>
	bool HasComponent(EntityID id) {
		return registry->HasComponent<T>(id);
	}
	template <typename T>
	T& GetComponent(EntityID id) {
		return registry->GetComponent<T>(id);
	}
	template <typename T, typename...Args>
	T* AddSystem(Args&&...args) {
		T* sys = new T(std::forward<Args>(args)...);
		systems.push_back(sys);
		return sys;
	}
	void Update(float dt) {
		for (auto* sys : systems) sys->Update(dt);
	}
	void Draw() {
		for (auto* sys : systems) sys->Draw();
	}
	Registry* GetRegistry() { return registry.get(); }

private:
	std::unique_ptr<Registry> registry;
	std::vector<System*> systems;
};