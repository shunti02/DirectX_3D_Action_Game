/*===================================================================
//ファイル:GameScene.h
//概要:ゲームプレイ中のシーン
=====================================================================*/
#pragma once
#include "BaseScene.h"
#include "Game/EntityFactory.h"
#include "ECS/Systems/RenderSystem.h"
#include "ECS/Systems/CameraSystem.h"
#include "ECS/Systems/PlayerSystem.h"
#include "ECS/Systems/PhysicsSystem.h"
#include <iostream>

class GameScene : public BaseScene {
public:
    using BaseScene::BaseScene; // コンストラクタ継承

    void Initialize() override {
        // システム登録
        pWorld->AddSystem<PlayerSystem>()->Init(pWorld.get());
        pWorld->AddSystem<PhysicsSystem>()->Init(pWorld.get());
        pWorld->AddSystem<CameraSystem>()->Init(pWorld.get());
        pWorld->AddSystem<RenderSystem>()->Init(pWorld.get());

        // --- エンティティ配置 (共通Factory利用) ---

        // 1. プレイヤー生成
        EntitySpawnParams playerParams;
        playerParams.type = "Player";
        playerParams.position = { 0.0f, 0.0f, 0.0f };
        playerParams.scale = { 1.0f, 1.0f, 1.0f };
        EntityID playerID = EntityFactory::CreateEntity(pWorld.get(), playerParams);

        // 2. カメラ生成
        EntitySpawnParams camParams;
        camParams.type = "Camera";
        camParams.position = { 0.0f, 0.0f, 0.0f };//元は（0.0f,3.0f,-8.0f）
        EntityID cameraID = EntityFactory::CreateEntity(pWorld.get(), camParams);

        // カメラの追従設定 (現状はFactory外で行う)
        auto& camComp = pWorld->GetComponent<CameraComponent>(cameraID);
        camComp.targetEntityID = playerID;
        camComp.distance = 10.0f;
        camComp.height = 4.0f;
        camComp.lookAtOffset = 0.0f;

        // 3. 敵生成
        EntitySpawnParams enemyParams;
        enemyParams.type = "Enemy";
        enemyParams.position = { 1.0f, 0.0f, 0.0f };
        enemyParams.scale = { 1.0f, 1.0f, 1.0f };
        EntityFactory::CreateEntity(pWorld.get(), enemyParams);

        // 3. 敵生成
        EntitySpawnParams enemy2Params;
        enemy2Params.type = "Enemy2";
        enemy2Params.position = { 1.0f, 0.0f, 3.0f };
        enemy2Params.scale = { 1.0f, 1.0f, 1.0f };
        EntityFactory::CreateEntity(pWorld.get(), enemy2Params);

        // 4. 地面生成 (スケールで大きくする)
        EntitySpawnParams groundParams;
        groundParams.type = "Ground";
        groundParams.position = { 0.0f, -1.0f, 0.0f };
        groundParams.scale = { 30.0f, 1.0f, 30.0f };
        EntityFactory::CreateEntity(pWorld.get(), groundParams);

        // 4. 地面生成 (スケールで大きくする)
        EntitySpawnParams ground2Params;
        ground2Params.type = "Ground2";
        ground2Params.position = { 14.5f, 0.0f, 0.0f };
        ground2Params.scale = { 1.0f, 1.0f, 30.0f };
        EntityFactory::CreateEntity(pWorld.get(), ground2Params);

        // 4. 地面生成 (スケールで大きくする)
        EntitySpawnParams ground3Params;
        ground3Params.type = "Ground3";
        ground3Params.position = { 0.0f, 0.0f, 14.5f };
        ground3Params.scale = { 30.0f, 1.0f, 1.0f };
        EntityFactory::CreateEntity(pWorld.get(), ground3Params);

        // 4. 地面生成 (スケールで大きくする)
        EntitySpawnParams ground4Params;
        ground4Params.type = "Ground4";
        ground4Params.position = { -14.5f, 0.0f, 0.0f };
        ground4Params.scale = { 1.0f, 1.0f, 30.0f };
        EntityFactory::CreateEntity(pWorld.get(), ground4Params);

#ifdef _DEBUG
        std::cout << "GameScene Initialized. Entities Created via Factory." << std::endl;
#endif
    }
};
