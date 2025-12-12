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
        // 1. システムの登録 (まだSystemが少ないのでコメントアウト)
        pWorld->AddSystem<PlayerSystem>()->Init(pWorld.get());
        pWorld->AddSystem<PhysicsSystem>()->Init(pWorld.get());
        pWorld->AddSystem<CameraSystem>()->Init(pWorld.get());
        pWorld->AddSystem<RenderSystem>()->Init(pWorld.get());

        //Entityの配置 (Factoryのおかげで一行で書ける！)
        //プレイヤー（立方体）を真ん中に
        EntityID playerID = EntityFactory::CreatePlayer(pWorld.get(), 0.0f, 0.0f, 0.0f);

        //カメラ配置 (手前 Z=-5.0f に配置して、原点を見る)
        EntityID cameraID = EntityFactory::CreateCamera(pWorld.get(), 0.0f, 3.0f, -8.0f);
        auto& camComp = pWorld->GetComponent<CameraComponent>(cameraID);
        camComp.targetEntityID = playerID; // ターゲットセット
        camComp.distance = 6.0f;           // 距離調整
        camComp.height = 3.0f;             // 高さ調整
        camComp.lookAtOffset = 1.0f;       // 腰を見る（1.0fくらい）
        
        // 敵（円錐）を右に
        EntityFactory::CreateEnemy(pWorld.get(), 3.0f, 0.0f, 0.0f);

        // プレイヤーの下に大きな箱（床）を置く
        EntityFactory::CreateGround(pWorld.get(), 0.0f, -2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 10.0f, 1.0f, 10.0f);

#ifdef _DEBUG
        std::cout << "GameScene Initialized. Entities Created." << std::endl;
#endif
    }
};
