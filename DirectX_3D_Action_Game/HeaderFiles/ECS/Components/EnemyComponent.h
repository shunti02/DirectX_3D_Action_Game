/*===================================================================
//ファイル:EnemyComponent.h
//概要:敵のパラメータ（速度や索敵範囲など）
=====================================================================*/
#pragma once
// 敵の種類
enum class EnemyType {
    Normal,
    Ranged,
    Heavy,
    Boss
};
// 敵の状態定義
enum class EnemyState {
    Chase,      // 追跡
    Strafing,   // 回り込み（横移動）
    Retreat,    // 後退（距離を取る）
    Attack,     // 攻撃中
    Cooldown,   // 攻撃後の硬直
    Stun,       // 被弾時のひるみ（ノックバック中）
    // --- ボス用 ---
    BossIdle,        // 待機
    BossRingBarrage, // 全方位弾幕
    BossBitLaser,    // ビット一斉射撃
    BossRapidFire    // ★追加: ボス狙い撃ち
};
struct EnemyComponent {
    EnemyType type = EnemyType::Normal;
    // 基本ステータス
    float moveSpeed = 3.0f;
    float attackRange = 2.5f;

    // 状態管理
    EnemyState state = EnemyState::Chase;
    float stateTimer = 0.0f;      // 現在の状態を維持するタイマー

    // 攻撃関連
    float attackTimer = 0.0f;     // 攻撃モーション用
    float attackDuration = 0.5f;
    float cooldownTime = 2.0f;

    // 遠距離タイプ用
    bool isRanged = false;
    float attackCooldownTimer = 0.0f;
    float attackInterval = 3.0f;
    float optimalRange = 15.0f;   // 理想的な交戦距離（遠距離用）

    // 物理・制御
    float knockbackTimer = 0.0f;
    float weight = 1.0f;
    bool isImmovable = false;

    // AI思考用
    float strafeDirection = 1.0f; // 1.0(右) or -1.0(左)
    float thinkInterval = 0.0f;   // 次に思考するまでの時間
    // ★追加: ボス用フェーズ管理
    int bossPhase = 1;
};
