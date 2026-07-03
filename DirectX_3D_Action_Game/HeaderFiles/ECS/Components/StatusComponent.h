#pragma once

struct StatusComponent {
	int hp = 100;
	int maxHp = 100;
	int attackPower = 10;
	float invincibleTimer = 0.0f;

	bool IsDead() const { return hp <= 0; }
	void TakeDamage(int damage) { hp -= damage; if (hp < 0) hp = 0; }
};