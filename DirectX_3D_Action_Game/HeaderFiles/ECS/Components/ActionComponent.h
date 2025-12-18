/*===================================================================
//ファイル:ActionComponent.h
//概要:アクション状態（攻撃中フラグ、クールタイムなど）
=====================================================================*/
#pragma once

struct ActionComponent {
	bool isAttacking;// 現在攻撃中か
	float attackTimer;// 攻撃判定が出ている時間管理
	float cooldownTimer;// 次の攻撃までの待ち時間

	//パラメーター
	float attackDuration; // 攻撃アニメーションの持続時間
	float attackCooldown; // クールタイム

	ActionComponent(float duration = 0.2f, float cooldown = 0.5f)
		:isAttacking(false), attackTimer(0.0f), cooldownTimer(0.0f),
		attackDuration(duration), attackCooldown(cooldown){ }
};
