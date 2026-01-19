#pragma once

struct BulletComponent {
    int damage;       // 威力
    float lifeTime;   // 消えるまでの時間
    bool isActive;    // 有効フラグ
    // ★追加: プレイヤーが撃った弾かどうか
    bool fromPlayer = false;
};
