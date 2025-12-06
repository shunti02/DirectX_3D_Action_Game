/*===================================================================
//ファイル:ECS.h
//制作日:2025/12/06
//概要:Registry(EntityとComponentの紐づけ管理)
//制作者:伊藤駿汰
//-------------------------------------------------------------------
//更新履歴:
//2025/12/06:新規作成
=====================================================================*/
#ifndef ECS_H
#define ECS_H
#include "Component.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <deque>
#include <typeindex>
#include <utility>

/*----------------------------------------------
//ComponentPool<T>:データ配列
-----------------------------------------------*/
template<typename T>
class ComponentPool : public IComponentPool {
public:
	ComponentPool(size_t capacity = ECSConfig::MAX_COMPONENTS) {
		data.resize(capacity);
	}
	//データのリセット
	void Set(EntityID entityID, T component) {
		if (entityID >= data.size()) {
			data.resize(ECSConfig::MAX_COMPONENTS);
		}
		data[entityID] = component;
	}
	//データの取得
	T& Get(EntityID entityID) {
		return data[entityID];
	}
	//Entityの削除処理
	void OnEntityDestroyed(EntityID entityID)override {

	}
private:
	std::vector<T> data;
};
/*---------------------------------------------------------
//Registry:Entity生成・破棄・コンポーネント紐づけの管理者
----------------------------------------------------------*/
class Registry {
public:
	Registry() {
		entityComponentMasks.resize(ECSConfig::MAX_COMPONENTS);
		//IDプールの初期化
		for (EntityID i = 0; i < ECSConfig::MAX_COMPONENTS; ++i) {
			freeEntities.push_back(i);
		}
	}
	//Entity生成
	EntityID CreateEntity() {
		if (freeEntities.empty()) return ECSConfig::INVALID_ID;

		EntityID id = freeEntities.front();
		freeEntities.pop_front();
		activeEntityCount++;
		return id;
	}
	//Entity削除
	void DestroyEntity(EntityID entity) {
		entityComponentMasks[entity].reset();
		freeEntities.push_back(entity);
		activeEntityCount--;

		//各プールに通知
		for (auto& pair : componentPools) {
			pair.second->OnEntityDestroyed(entity);
		}
	}
	//コンポーネント追加
	template <typename T, typename...Args>
	void AddComponent(EntityID entity, Args&&...args) {
		const auto componentID = ComponentType<T>::GetID();

		//データを取得してデータをリセット
		auto pool = GetComponentPool<T>();
		pool->Set(entity, T(std::forward<Args>(args)...));
		//マスクをオン
		entityComponentMasks[entity].set(componentID);
	}
	//コンポーネント取得
	template <typename T>
	T& GetComponent(EntityID entity) {
		auto pool = GetComponentPool<T>();
		return pool->Get(entity);
	}
	//コンポーネントを持っているかどうか
	template <typename T>
	bool HasComponent(EntityID entity)const {
		const auto componentID = ComponentType<T>::GetID();
		return entityComponentMasks[entity].test(componentID);
	}
private:
	//型ごとのプールを取得または作成
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
	std::vector<ComponentMask> entityComponentMasks;//誰が何を持っているか
	std::unordered_map<const char*, std::shared_ptr<IComponentPool>> componentPools;
};

#endif //ECS_H
