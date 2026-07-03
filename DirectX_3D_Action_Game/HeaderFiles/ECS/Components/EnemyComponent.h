#pragma once
enum class EnemyType {
    Normal,
    Ranged,
    Heavy,
    Boss
};
enum class EnemyState {
    Chase,
    Strafing,
    Retreat,
    Attack,
    Cooldown,
    Stun,
    BossIdle,
    BossRingBarrage,
    BossBitLaser,
    BossRapidFire 
};
struct EnemyComponent {
    EnemyType type = EnemyType::Normal;
    float moveSpeed = 3.0f;
    float attackRange = 2.5f;

    EnemyState state = EnemyState::Chase;
    float stateTimer = 0.0f;
    float attackTimer = 0.0f;
    float attackDuration = 0.5f;
    float cooldownTime = 2.0f;

    bool isRanged = false;
    float attackCooldownTimer = 0.0f;
    float attackInterval = 3.0f;
    float optimalRange = 15.0f;
    float knockbackTimer = 0.0f;
    float weight = 1.0f;
    bool isImmovable = false;

    float strafeDirection = 1.0f;
    float thinkInterval = 0.0f;
    int bossPhase = 1;
};
