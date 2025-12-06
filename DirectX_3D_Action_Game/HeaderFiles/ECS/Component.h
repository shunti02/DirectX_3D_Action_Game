/*===================================================================
//ファイル:Component.h
//制作日:2025/12/05
//概要:EntityID定義、コンポーネント管理基盤
//制作者:伊藤駿汰
//-------------------------------------------------------------------
//更新履歴:
//2025/12/05:新規作成
=====================================================================*/
#ifndef COMPONENT_H
#define COMPONENT_H
#include <vector>
#include <bitset>
#include <cstdint>
/*--------------------------------------------------
//Entity定義
----------------------------------------------------*/
using EntityID = std::uint32_t;
//設定
namespace ECSConfig {
	constexpr EntityID MAX_ENTITIES = 5000;//最大エンティティ数
	constexpr std::uint32_t MAX_COMPONENTS = 32;//コンポーネントの種類数
	constexpr EntityID INVALID_ID = MAX_ENTITIES;
}


/*--------------------------------------------------------------
//Component管理基盤
---------------------------------------------------------------*/
//ComponentMask
using ComponentMask = std::bitset<ECSConfig::MAX_COMPONENTS>;
//ComponentID発行カウンター
struct ComponentTypeCounter {
	static std::uint32_t counter;
};
template <typename T>
struct ComponentType {
	static std::uint32_t GetID() {
		static std::uint32_t id = ComponentTypeCounter::counter++;
		return id;
	}
};

//配列管理用インターフェース
class IComponentPool {
public:
	virtual ~IComponentPool() = default;
	virtual void OnEntityDestroyed(EntityID entityID) = 0;
};
#endif //COMPONENT_H
