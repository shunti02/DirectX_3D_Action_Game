#pragma once
#include <DirectXMath.h>

enum class ParticleType {
    Spark,
    Smoke,
    Explosion,
    MuzzleFlash 
};

struct ParticleComponent {
    float lifeTime; 
    float maxLifeTime; 

    DirectX::XMFLOAT3 velocity;

    bool useGravity;
    float scaleSpeed;
    ParticleType type = ParticleType::Spark;
};