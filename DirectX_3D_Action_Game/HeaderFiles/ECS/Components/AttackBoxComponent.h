/*===================================================================
//ファイル:AttackBoxComponent.h
//概要:攻撃判定であることを示すタグ＆データ
=====================================================================*/
#pragma once

#include "ECS/ECS.h"
struct AttackBoxComponent {
	EntityID ownerID;// 誰が放った攻撃か
	int damage;// 攻撃力
	float lifeTime; // 生存時間

	AttackBoxComponent(EntityID owner = ECSConfig::INVALID_ID, int damage = 1, float time = 0.2f)
		:ownerID(owner), damage(damage), lifeTime(time){ }
};
