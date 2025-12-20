#pragma once

struct AttackSphereComponent {
    int ownerID = -1;       // 誰が撃ったか
    int damage = 0;         // ダメージ量
    float lifeTime = 0.5f;  // 消えるまでの時間

    float currentRadius = 0.5f;   // 現在の大きさ
    float maxRadius = 3.0f;       // 最大の大きさ
    float expansionSpeed = 10.0f; // 広がる速さ
};