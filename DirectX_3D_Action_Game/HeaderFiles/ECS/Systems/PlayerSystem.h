#pragma once
#include "ECS/System.h"

// 前方宣言 (インクルード削減のため)
class Registry;

class PlayerSystem : public System {
public:
    // 連打防止用フラグ
    bool tabKeyPressed = false;

    void Update(float dt) override;

private:
    // キャラクター切り替え処理
    void SwitchCharacter(Registry* registry);
};
