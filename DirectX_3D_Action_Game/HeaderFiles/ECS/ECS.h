#pragma once
#include "Component.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <deque>
#include <typeindex>
#include <utility>

template<typename T>
class ComponentPool : public IComponentPool {
public:
	ComponentPool(size_t capacity = ECSConfig::MAX_ENTITIES) {
		data.resize(capacity);
	}
	void Set(EntityID entityID, T component) {
		if (entityID >= data.size()) {
			data.resize(ECSConfig::MAX_ENTITIES);
		}
		data[entityID] = component;
	}
	T& Get(EntityID entityID) {
		return data[entityID];
	}
	void OnEntityDestroyed(EntityID entityID)override {

	}
private:
	std::vector<T> data;
};
class Registry {
public:
	Registry() {
		entityComponentMasks.resize(ECSConfig::MAX_ENTITIES);
		for (EntityID i = 0; i < ECSConfig::MAX_ENTITIES; ++i) {
			freeEntities.push_back(i);
		}
	}
	//Entity¶¬
	EntityID CreateEntity() {
		if (freeEntities.empty()) return ECSConfig::INVALID_ID;

		EntityID id = freeEntities.front();
		freeEntities.pop_front();
		activeEntityCount++;
		return id;
	}
	//Entityíœ
	void DestroyEntity(EntityID entity) {
		if (entity >= entityComponentMasks.size()) return;
		entityComponentMasks[entity].reset();
		freeEntities.push_back(entity);
		activeEntityCount--;

		for (auto& pair : componentPools) {
			pair.second->OnEntityDestroyed(entity);
		}
	}

	template <typename T, typename...Args>
	void AddComponent(EntityID entity, Args&&...args) {
		const auto componentID = ComponentType<T>::GetID();

		auto pool = GetComponentPool<T>();

		pool->Set(entity, T{ std::forward<Args>(args)... });

		entityComponentMasks[entity].set(componentID);
	}
	template <typename T>
	T& GetComponent(EntityID entity) {
		auto pool = GetComponentPool<T>();
		return pool->Get(entity);
	}
	template <typename T>
	bool HasComponent(EntityID entity)const {
		if (entity >= entityComponentMasks.size()) return false;
		const auto componentID = ComponentType<T>::GetID();
		return entityComponentMasks[entity].test(componentID);
	}
private:
	template <typename T>
	std::shared_ptr<ComponentPool<T>>GetComponentPool() {
		const char* typeName = typeid(T).name();

		if (componentPools.find(typeName) == componentPools.end()) {
			componentPools[typeName] = std::make_shared<ComponentPool<T>>();
		}
		return std::static_pointer_cast<ComponentPool<T>>(componentPools[typeName]);
	}

	std::uint32_t activeEntityCount = 0;
	std::deque<EntityID> freeEntities;
	std::vector<ComponentMask> entityComponentMasks;
	std::unordered_map<const char*, std::shared_ptr<IComponentPool>> componentPools;
};
