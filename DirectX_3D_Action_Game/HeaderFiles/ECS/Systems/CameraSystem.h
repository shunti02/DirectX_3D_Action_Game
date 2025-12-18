#pragma once
#include "ECS/System.h"

// 必要であれば前方宣言を行うことでインクルードをさらに減らせますが、
// ここではSystemの継承に必要なヘッダーのみインクルードしています。

class CameraSystem : public System {
public:
    // コンストラクタやデストラクタが必要な場合はここに宣言します
    // CameraSystem();
    // ~CameraSystem() override;

    void Update(float dt) override;
};