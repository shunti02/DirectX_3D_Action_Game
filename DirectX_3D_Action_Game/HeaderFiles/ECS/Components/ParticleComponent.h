#pragma once
#include <DirectXMath.h>

struct ParticleComponent {
    float lifeTime;       // 生存時間 (秒)
    float maxLifeTime;    // 初期寿命 (フェードアウト計算用)

    DirectX::XMFLOAT3 velocity; // 飛んでいく方向と速さ

    bool useGravity;      // 重力で落ちるか？ (煙ならfalse, 岩ならtrue)
    float scaleSpeed;     // サイズ変化速度 (マイナスなら徐々に小さくなる)
};