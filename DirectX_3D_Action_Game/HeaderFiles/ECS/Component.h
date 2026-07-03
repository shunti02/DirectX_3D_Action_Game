#ifndef COMPONENT_H
#define COMPONENT_H
#include <vector>
#include <bitset>
#include <cstdint>
/*--------------------------------------------------
//EntityíËã`
----------------------------------------------------*/
using EntityID = std::uint32_t;
namespace ECSConfig {
	constexpr EntityID MAX_ENTITIES = 5000;
	constexpr std::uint32_t MAX_COMPONENTS = 32;
	constexpr EntityID INVALID_ID = MAX_ENTITIES;
}


/*--------------------------------------------------------------
//Componentä«óù
---------------------------------------------------------------*/
//ComponentMask
using ComponentMask = std::bitset<ECSConfig::MAX_COMPONENTS>;
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

class IComponentPool {
public:
	virtual ~IComponentPool() = default;
	virtual void OnEntityDestroyed(EntityID entityID) = 0;
};
#endif
