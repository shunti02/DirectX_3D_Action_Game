/*===================================================================
//ファイル:StatusComponent.h
//概要:キャラクターのステータス（HP, 攻撃力など）
=====================================================================*/
#pragma once

struct StatusComponent {
	int hp;
	int maxHp;
	int attackPower;
	float invincibleTimer;

	//コンストラクタ
	StatusComponent(int _hp = 10, int _atk = 1)
		:hp(_hp), maxHp(_hp), attackPower(_atk), invincibleTimer(0.0f){ }
	//ダメージを受ける関数
	void TakeDamage(int damage) {
		hp -= damage;
		if (hp < 0)hp = 0;
	}

	//生きているか
	bool IsDead()const { return hp <= 0; }
};