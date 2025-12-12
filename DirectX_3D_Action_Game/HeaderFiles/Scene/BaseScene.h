/*===================================================================
//ファイル:BaseScene.h
//概要:全てのシーンの基底クラス。独自のWorld(ECS)を持つ。
=====================================================================*/
#pragma once
#include <memory>
#include "ECS/World.h"

class SceneManager; // 前方宣言

class BaseScene {
public:
    BaseScene(SceneManager* manager) : pManager(manager) {
        // シーン生成時に、このシーン専用のECSワールドを作る
        pWorld = std::make_unique<World>();
    }
    virtual ~BaseScene() = default;

    // 純粋仮想関数：子クラスで必ず実装
    virtual void Initialize() = 0;

    virtual void Update(float dt) {
        if (pWorld) pWorld->Update(dt);
    }

    virtual void Draw() {
        if (pWorld) pWorld->Draw();
    }

    virtual void Shutdown() {
        // 必要なら破棄処理
    }

protected:
    SceneManager* pManager;     // シーン遷移用
    std::unique_ptr<World> pWorld; // このシーン専用のECS
};
