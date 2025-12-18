/*===================================================================
//ファイル:EnemyComponent.h
//概要:敵のパラメータ（速度や索敵範囲など）
=====================================================================*/
#pragma once

struct EnemyComponent {
	float moveSpeed;       // 移動速度
	float stopDistance;  // 停止距離

	EnemyComponent(float speed = 2.0f, float stopDist = 1.2f)
		:moveSpeed(speed), stopDistance(stopDist){ }
};
