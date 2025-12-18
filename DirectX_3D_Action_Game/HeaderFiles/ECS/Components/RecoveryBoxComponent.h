/*===================================================================
//ファイル:RecoveryBoxComponent.h
//概要:回復判定であることを示すタグ＆データ
=====================================================================*/
#pragma once

#include "ECS/ECS.h"

struct RecoveryBoxComponent {
	EntityID ownerID;// 誰が放った回復か
	int healAmount;// 回復量
	float lifeTime;// 生存時間

	RecoveryBoxComponent(EntityID owner = ECSConfig::INVALID_ID, int heal = 10, float time = 0.5f)
		:ownerID(owner), healAmount(heal), lifeTime(time){ }
};
