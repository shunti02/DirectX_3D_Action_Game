/*= ==================================================================
//ファイル:EnemySystem.h
//概要:敵の思考と移動制御（プレイヤー追跡）
==================================================================== = */
#pragma once
#include "ECS/System.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/EnemyComponent.h"
#include "App/Game.h"
#include <DirectXMath.h>
#include <cmath>

using namespace DirectX;

class EnemySystem : public System {
public:
	void Update(float dt)override {
		auto registry = pWorld->GetRegistry();

		//ターゲットを探す
		EntityID playerID = ECSConfig::INVALID_ID;
		for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
			if (registry->HasComponent<PlayerComponent>(id)) {
				playerID = id;
				break;
			}
		}
		//プレイヤーがいなければ何もしない
		if (playerID == ECSConfig::INVALID_ID)return;
		//プレイヤーの位置を取得
		auto& pTrans = registry->GetComponent<TransformComponent>(playerID);
		XMVECTOR targetPos = XMLoadFloat3(&pTrans.position);

		//全ての敵に対して処理を行う
		for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
			//EnemyComponentを持っていなければスキップ
			if (!registry->HasComponent<EnemyComponent>(id))continue;

			auto& eTrans = registry->GetComponent<TransformComponent>(id);
			auto& eComp = registry->GetComponent<EnemyComponent>(id);

			XMVECTOR enemyPos = XMLoadFloat3(&eTrans.position);

			//ベクトル計算
			XMVECTOR diff = targetPos - enemyPos;

			//平面移動させる
			diff = XMVectorSetY(diff, 0.0f);

			//距離の二乗を計算
			float distSq = XMVectorGetX(XMVector3LengthSq(diff));
			float stopDistSq = eComp.stopDistance * eComp.stopDistance;

			//停止位置より遠ければ近づく
			if (distSq > stopDistSq) {
				//正規化
				XMVECTOR dir = XMVector3Normalize(diff);
				//移動量計算
				XMVECTOR velocity = dir * eComp.moveSpeed * dt;
				XMVECTOR newPos = enemyPos + velocity;
				XMStoreFloat3(&eTrans.position, newPos);
				//向きの制御
				float angle = atan2f(XMVectorGetX(dir), XMVectorGetZ(dir));

				//Y軸回転
				eTrans.rotation.y = angle;
			}
		}
	}
};