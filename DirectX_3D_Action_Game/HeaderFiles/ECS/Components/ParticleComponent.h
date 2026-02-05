#pragma once
#include <DirectXMath.h>

enum class ParticleType {
    Spark,      // 火花（重力あり、跳ねる）
    Smoke,      // 煙（上に昇る、減速する）
    Explosion,  // 爆発（全方位に広がる）
    MuzzleFlash // 銃口の閃光（一瞬で消える）
};

struct ParticleComponent {
    float lifeTime;       // 生存時間 (秒)
    float maxLifeTime;    // 初期寿命 (フェードアウト計算用)

    DirectX::XMFLOAT3 velocity; // 飛んでいく方向と速さ

    bool useGravity;      // 重力で落ちるか？ (煙ならfalse, 岩ならtrue)
    float scaleSpeed;     // サイズ変化速度 (マイナスなら徐々に小さくなる)
    ParticleType type = ParticleType::Spark;
};