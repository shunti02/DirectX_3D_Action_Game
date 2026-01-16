/*===================================================================
//ファイル:EnemyComponent.h
//概要:敵のパラメータ（速度や索敵範囲など）
=====================================================================*/
#pragma once
// 敵の状態定義
enum class EnemyState {
	Chase,      // 追跡
	Attack,     // 攻撃中
	Cooldown    // 攻撃後の硬直
};
struct EnemyComponent {
    float moveSpeed = 3.0f;
    float attackRange = 2.5f;
    float attackTimer = 0.0f;
    float attackDuration = 0.5f;
    float cooldownTime = 2.0f;
    EnemyState state = EnemyState::Chase;

    bool isRanged = false;            // 遠距離タイプかどうか
    float attackCooldownTimer = 0.0f; // 射撃のクールダウン
    float attackInterval = 3.0f;      // 射撃間隔（秒）

    float knockbackTimer = 0.0f;
    float weight = 1.0f;       // 1.0が標準。大きいほど吹き飛びにくい
    bool isImmovable = false;  // trueなら絶対に動かない（ラスボス用）
};
