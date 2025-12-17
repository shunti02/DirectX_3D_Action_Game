/*===================================================================
//ファイル:World.h
//制作日:2025/12/06
//概要:ECSの一括管理
//制作者:伊藤駿汰
//-------------------------------------------------------------------
//更新履歴:
//2025/12/06:新規作成
=====================================================================*/
#ifndef WORLD_H
#define WORLD_H
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
	/*----------------------------------------------
	//EntityBuilder:生成時のメソッドチェーン用ヘルパー
	-------------------------------------------------*/
	class EntityBuilder {
	public:
		EntityBuilder(World* world, EntityID id) : pWorld(world),entityID(id){}

		//一括作成機能
		template <typename T, typename...Args>
		EntityBuilder& AddComponent(Args&&...args) {
			pWorld->registry->AddComponent<T>(entityID, std::forward<Args>(args)...);
			return *this;
		}
		//最後にIDを返す
		EntityID Build() const { return entityID; }
	private:
		World* pWorld;
		EntityID entityID;
	};
	/*----------------------------------------------------------------
	//Entity操作
	-----------------------------------------------------------------*/
	//Entity操作開始
	EntityBuilder CreateEntity() {
		EntityID id = registry->CreateEntity();
		return EntityBuilder(this, id);
	}
	//コンポーネント追加のラッパー関数
	template <typename T, typename...Args>
	void AddComponent(EntityID id, Args&&...args) {
		registry->AddComponent<T>(id, std::forward<Args>(args)...);
	}
	//Entity削除
	void DestroyEntity(EntityID id) {
		registry->DestroyEntity(id);
	}
	//HasComponent
	template <typename T>
	bool HasComponent(EntityID id) {
		return registry->HasComponent<T>(id);
	}
	//Component取得
	template <typename T>
	T& GetComponent(EntityID id) {
		return registry->GetComponent<T>(id);
	}
	/*-----------------------------------------------------------------
	//System管理
	-------------------------------------------------------------------*/
	//System登録
	template <typename T, typename...Args>
	T* AddSystem(Args&&...args) {
		T* sys = new T(std::forward<Args>(args)...);
		systems.push_back(sys);
		return sys;
	}
	//一括更新
	void Update(float dt) {
		for (auto* sys : systems) sys->Update(dt);
	}
	//一括描画
	void Draw() {
		for (auto* sys : systems) sys->Draw();
	}

	//Registryへの直接アクセス
	Registry* GetRegistry() { return registry.get(); }

private:
	std::unique_ptr<Registry> registry;
	std::vector<System*> systems;
};

#endif //WORLD_H