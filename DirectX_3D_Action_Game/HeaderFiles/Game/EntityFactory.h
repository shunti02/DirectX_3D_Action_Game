#pragma once
#include "App/Main.h"
#include "ECS/World.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/CameraComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/EnemyComponent.h"
#include "ECS/Components/ColliderComponent.h"
#include "ECS/Components/StatusComponent.h"
#include "ECS/Components/ActionComponent.h"
#include "ECS/Components/AttackBoxComponent.h"
#include "ECS/Components/RecoveryBoxComponent.h"
#include "ECS/Components/AttackSphereComponent.h"
#include "ECS/Components/RecoverySphereComponent.h"
#include "ECS/Components/RolesComponent.h"
#include "ECS/Components/PlayerPartComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include "ECS/Components/BulletComponent.h"
#include "ECS/Components/ParticleComponent.h"
#include "ECS/Components/EnemyPartComponent.h"
#include "App/Game.h"
#include "Engine/GeometryGenerator.h"
#include "Engine/Vertex.h"
#include "Engine/Colors.h"
#include <vector>
#include <string>
#include <iostream>
#include <DirectXMath.h>

enum class PlayerRole { Attacker, Healer };

struct EntitySpawnParams {
    std::string type;
    DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 rotation = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
    DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    PlayerRole role = PlayerRole::Attacker;
    PlayerType playerType = PlayerType::AssaultStriker;
    bool isGrounded = false;
    std::string name = "";
};

namespace EntityFactory {

    inline void AttachMeshAndCollider(EntityID id, World* world, ShapeType shape, DirectX::XMFLOAT4 color, ColliderType colType, float cx, float cy, float cz) {
        MeshData data = GeometryGenerator::CreateMesh(shape, color);
        auto& mesh = world->GetComponent<MeshComponent>(id);
        mesh.vertexCount = (UINT)data.vertices.size();
        mesh.indexCount = (UINT)data.indices.size();
        mesh.stride = sizeof(Vertex);

        Graphics* g = Game::GetInstance()->GetGraphics();
        g->CreateVertexBuffer(data.vertices, mesh.pVertexBuffer.GetAddressOf());
        g->CreateIndexBuffer(data.indices, mesh.pIndexBuffer.GetAddressOf());

        if (colType != ColliderType::Type_None) {
            auto& col = world->GetComponent<ColliderComponent>(id);
            if (colType == ColliderType::Type_Box) col.SetBox(cx, cy, cz);
            else if (colType == ColliderType::Type_Capsule) col.SetCapsule(cx, cy);
            else if (colType == ColliderType::Type_Sphere) col.SetSphere(cx);
        }
    }

    inline EntityID CreateEntity(World* world, const EntitySpawnParams& params) {
        TransformComponent transformData{ .position = params.position, .rotation = params.rotation, .scale = params.scale };
        EntityID id = world->CreateEntity().AddComponent<TransformComponent>(transformData).Build();

        if (params.type == "Player") {
            AppLog::AddLog("[Warning] ƒvƒŒƒCƒ„[¶¬‚É‚Í AssembleCustomPlayer ‚ðŽg—p‚µ‚Ä‚­‚¾‚³‚¢B");
        }
        else if (params.type == "EnemyMelee") {
        }
        else if (params.type == "EnemySniper") {
        }
        else if (params.type == "EnemyTurret") {
        }
        else if (params.type == "Ground") {
            world->AddComponent<                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             MeshComponent>(id);
            world->AddComponent<ColliderComponent>(id);
            DirectX::XMFLOAT4 color = { 0.15f, 0.2f, 0.25f, 1.0f };
            AttachMeshAndCollider(id, world, ShapeType::CYLINDER, color, ColliderType::Type_Box, 1.0f, 1.0f, 1.0f);
        }
        else if (params.type == "Boundary") {
            world->AddComponent<MeshComponent>(id);
            DirectX::XMFLOAT4 alertColor = { 1.0f, 0.0f, 0.2f, 0.3f };
            AttachMeshAndCollider(id, world, ShapeType::CUBE, alertColor, ColliderType::Type_None, 0, 0, 0);
        }
        else if (params.type == "Camera") {
            world->AddComponent<CameraComponent>(id);
        }
        return id;
    }

    inline void CreateAttackHitbox(World* world, EntityID ownerID, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 scale, int damage, float lifeTime = 0.1f) {
        EntityID id = world->CreateEntity()
            .AddComponent<TransformComponent>(TransformComponent{ .position = pos, .scale = scale })
            .AddComponent<ColliderComponent>()
            .AddComponent<AttackBoxComponent>(AttackBoxComponent{ .ownerID = (int)ownerID, .damage = damage, .lifeTime = lifeTime }) // ©‚±‚±‚ðC³
            .Build();
        auto& col = world->GetComponent<ColliderComponent>(id);
        col.SetBox(1.0f, 1.0f, 1.0f);
        AttachMeshAndCollider(id, world, ShapeType::CUBE, Colors::Red, ColliderType::Type_Box, 1.0f, 1.0f, 1.0f);
    }

    inline void CreateAttackSphere(World* world, EntityID ownerID, DirectX::XMFLOAT3 pos, int damage) {
        EntityID id = world->CreateEntity()
            .AddComponent<TransformComponent>(TransformComponent{ .position = pos, .scale = {2.5f, 2.5f, 2.5f} })
            .AddComponent<AttackSphereComponent>(AttackSphereComponent{ .ownerID = (int)ownerID, .damage = damage, .lifeTime = 0.3f, .currentRadius = 0.5f, .maxRadius = 3.5f, .expansionSpeed = 15.0f })
            .Build();
        world->AddComponent<ColliderComponent>(id);
        AttachMeshAndCollider(id, world, ShapeType::SPHERE, Colors::White, ColliderType::Type_Sphere, 0.5f, 0.0f, 0.0f);
    }

    inline void CreateEnemyBullet(World* world, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 dir, int damage) {
        EntityID id = world->CreateEntity()
            .AddComponent<TransformComponent>(TransformComponent{ .position = pos, .scale = {0.3f, 0.3f, 0.3f} })
            .AddComponent<BulletComponent>(BulletComponent{ .damage = damage, .lifeTime = 5.0f, .isActive = true })
            .Build();
        world->AddComponent<MeshComponent>(id);
        world->AddComponent<ColliderComponent>(id);
        AttachMeshAndCollider(id, world, ShapeType::SPHERE, Colors::Red, ColliderType::Type_Sphere, 0.3f, 0.0f, 0.0f);
        if (!world->GetRegistry()->HasComponent<PhysicsComponent>(id)) {
            float speed = 10.0f;
            world->AddComponent<PhysicsComponent>(id, PhysicsComponent{ .velocity = { dir.x * speed, dir.y * speed, dir.z * speed }, .useGravity = false });
        }
    }

    inline void CreatePlayerBullet(World* world, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 dir, int damage) {
        EntityID id = world->CreateEntity()
            .AddComponent<TransformComponent>(TransformComponent{ .position = pos, .scale = {0.4f, 0.4f, 0.8f} })
            .AddComponent<BulletComponent>(BulletComponent{ .damage = damage, .lifeTime = 3.0f, .isActive = true, .fromPlayer = true })
            .Build();
        world->AddComponent<MeshComponent>(id);
        world->AddComponent<ColliderComponent>(id);
        AttachMeshAndCollider(id, world, ShapeType::SPHERE, { 0.0f, 1.0f, 0.8f, 1.0f }, ColliderType::Type_Sphere, 0.8f, 0.0f, 0.0f);
        if (!world->GetRegistry()->HasComponent<PhysicsComponent>(id)) {
            float speed = 25.0f;
            world->AddComponent<PhysicsComponent>(id, PhysicsComponent{ .velocity = { dir.x * speed, dir.y * speed, dir.z * speed }, .useGravity = false });
        }
    }

    inline void CreateExplosion(World* world, DirectX::XMFLOAT3 pos, int count, DirectX::XMFLOAT4 color) {
        for (int i = 0; i < count; ++i) {
            float speed = 5.0f + (rand() % 100) / 10.0f;
            float angleY = (rand() % 360) * 3.14f / 180.0f;
            float angleV = ((rand() % 180) - 90) * 3.14f / 180.0f;
            float vx = cosf(angleV) * sinf(angleY) * speed;
            float vy = sinf(angleV) * speed;
            float vz = cosf(angleV) * cosf(angleY) * speed;
            EntityID id = world->CreateEntity()
                .AddComponent<TransformComponent>(TransformComponent{ .position = pos, .scale = {0.5f, 0.5f, 0.5f} })
                .AddComponent<ParticleComponent>(ParticleComponent{ .lifeTime = 0.5f + (rand() % 50) / 100.0f, .velocity = {vx, vy, vz}, .useGravity = false, .scaleSpeed = -2.0f, .type = ParticleType::Explosion })
                .Build();
            world->AddComponent<MeshComponent>(id);
            AttachMeshAndCollider(id, world, ShapeType::CUBE, color, ColliderType::Type_None, 0, 0, 0);
        }
    }

    inline void CreateSmoke(World* world, DirectX::XMFLOAT3 pos, int count, DirectX::XMFLOAT4 color) {
        for (int i = 0; i < count; ++i) {
            float vx = (rand() % 100 - 50) / 20.0f;
            float vy = (rand() % 100) / 20.0f + 1.0f;
            float vz = (rand() % 100 - 50) / 20.0f;
            EntityID id = world->CreateEntity()
                .AddComponent<TransformComponent>(TransformComponent{ .position = pos, .scale = {0.8f, 0.8f, 0.8f} })
                .AddComponent<ParticleComponent>(ParticleComponent{ .lifeTime = 1.0f + (rand() % 10) / 10.0f, .velocity = {vx, vy, vz}, .useGravity = false, .scaleSpeed = -0.3f, .type = ParticleType::Smoke })
                .Build();
            world->AddComponent<MeshComponent>(id);
            AttachMeshAndCollider(id, world, ShapeType::CUBE, color, ColliderType::Type_None, 0, 0, 0);
        }
    }

    inline void CreateMuzzleFlash(World* world, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 dir) {
        pos.x += dir.x * 0.5f; pos.y += dir.y * 0.5f; pos.z += dir.z * 0.5f;
        EntityID id = world->CreateEntity()
            .AddComponent<TransformComponent>(TransformComponent{ .position = pos, .scale = {0.5f, 0.5f, 0.5f} })
            .AddComponent<ParticleComponent>(ParticleComponent{ .lifeTime = 0.1f, .velocity = {0, 0, 0}, .useGravity = false, .scaleSpeed = 5.0f, .type = ParticleType::MuzzleFlash })
            .Build();
        world->AddComponent<MeshComponent>(id);
        AttachMeshAndCollider(id, world, ShapeType::SPHERE, { 1.0f, 0.8f, 0.2f, 0.8f }, ColliderType::Type_None, 0, 0, 0);
    }

    inline void CreateHitEffect(World* world, DirectX::XMFLOAT3 pos, int count, DirectX::XMFLOAT4 color) {
        for (int i = 0; i < count; ++i) {
            float vx = (rand() % 100 - 50) / 10.0f;
            float vy = (rand() % 100) / 10.0f + 2.0f;
            float vz = (rand() % 100 - 50) / 10.0f;
            EntityID id = world->CreateEntity()
                .AddComponent<TransformComponent>(TransformComponent{ .position = pos, .scale = {0.2f, 0.2f, 0.2f} })
                .AddComponent<ParticleComponent>(ParticleComponent{ .lifeTime = 0.5f + (rand() % 10) / 20.0f, .velocity = {vx, vy, vz}, .useGravity = true, .scaleSpeed = -0.5f })
                .Build();
            world->AddComponent<MeshComponent>(id);
            AttachMeshAndCollider(id, world, ShapeType::TETRAHEDRON, color, ColliderType::Type_None, 0.0f, 0.0f, 0.0f);
        }
    }

    inline PartStatus GetPartStatus(PartType type, int modelID) {
        PartStatus s = { 10.0f, 1.0f, 1.0f, 0.0f, 5.0f };
        if (type == PartType::Head) {
            if (modelID == 0) { s.hp = 15.0f; s.attack = 2.0f;  s.defense = 1.0f;  s.speed = 0.0f;  s.weight = 5.0f; }
            if (modelID == 1) { s.hp = 30.0f; s.attack = 5.0f;  s.defense = 3.0f;  s.speed = -0.5f; s.weight = 12.0f; }
            if (modelID == 2) { s.hp = 10.0f; s.attack = 1.0f;  s.defense = 0.5f;  s.speed = 1.5f;  s.weight = 3.0f; }
            if (modelID == 3) { s.hp = 8.0f;  s.attack = 4.0f;  s.defense = 0.2f;  s.speed = 2.0f;  s.weight = 2.5f; }
            if (modelID == 4) { s.hp = 35.0f; s.attack = 1.0f;  s.defense = 5.0f;  s.speed = -1.0f; s.weight = 15.0f; } 
        }
        else if (type == PartType::Body) {
            if (modelID == 0) { s.hp = 40.0f; s.attack = 3.0f;  s.defense = 4.0f;  s.speed = 0.0f;  s.weight = 15.0f; }
            if (modelID == 1) { s.hp = 70.0f; s.attack = 5.0f;  s.defense = 8.0f;  s.speed = -1.0f; s.weight = 30.0f; }
            if (modelID == 2) { s.hp = 30.0f; s.attack = 2.0f;  s.defense = 2.0f;  s.speed = 2.0f;  s.weight = 10.0f; }
            if (modelID == 3) { s.hp = 25.0f; s.attack = 8.0f;  s.defense = 1.0f;  s.speed = 3.0f;  s.weight = 8.0f; }
            if (modelID == 4) { s.hp = 90.0f; s.attack = 2.0f;  s.defense = 12.0f; s.speed = -2.0f; s.weight = 40.0f; }
        }
        else if (type == PartType::ArmLeft || type == PartType::ArmRight) {
            if (modelID == 0) { s.hp = 15.0f; s.attack = 4.0f;  s.defense = 2.0f;  s.speed = 0.0f;  s.weight = 5.0f; }
            if (modelID == 1) { s.hp = 25.0f; s.attack = 8.0f;  s.defense = 4.0f;  s.speed = -0.5f; s.weight = 10.0f; }
            if (modelID == 2) { s.hp = 10.0f; s.attack = 3.0f;  s.defense = 1.0f;  s.speed = 0.5f;  s.weight = 3.0f; }
            if (modelID == 3) { s.hp = 8.0f;  s.attack = 10.0f; s.defense = 0.5f;  s.speed = 1.0f;  s.weight = 2.0f; }
            if (modelID == 4) { s.hp = 30.0f; s.attack = 5.0f;  s.defense = 6.0f;  s.speed = -1.0f; s.weight = 12.0f; }
        }
        else if (type == PartType::LegLeft || type == PartType::LegRight) {
            if (modelID == 0) { s.hp = 15.0f; s.attack = 1.0f;  s.defense = 3.0f;  s.speed = 5.0f;  s.weight = 10.0f; }
            if (modelID == 1) { s.hp = 25.0f; s.attack = 2.0f;  s.defense = 6.0f;  s.speed = 3.0f;  s.weight = 20.0f; }
            if (modelID == 2) { s.hp = 10.0f; s.attack = 0.0f;  s.defense = 1.5f;  s.speed = 8.0f;  s.weight = 5.0f; }
            if (modelID == 3) { s.hp = 10.0f; s.attack = 5.0f;  s.defense = 1.0f;  s.speed = 10.0f; s.weight = 4.0f; }
            if (modelID == 4) { s.hp = 35.0f; s.attack = 1.0f;  s.defense = 8.0f;  s.speed = 1.0f;  s.weight = 25.0f; }
        }
        return s;
    }
inline EntityID AssembleCustomPlayer(World* world, const EntitySpawnParams& params, const CustomizeData& cData, bool isEnemy = false) {
    EntityID playerCore = world->CreateEntity().Build();
    TransformComponent trans;
    trans.position = params.position;
    trans.scale = params.scale;
    world->AddComponent<TransformComponent>(playerCore, trans);

    PartStatus headS = GetPartStatus(PartType::Head, cData.headID);
    PartStatus bodyS = GetPartStatus(PartType::Body, cData.bodyID);
    PartStatus waistS = GetPartStatus(PartType::Waist, cData.waistID);
    PartStatus armLS = GetPartStatus(PartType::ArmLeft, cData.armLeftID);
    PartStatus armRS = GetPartStatus(PartType::ArmRight, cData.armRightID);
    PartStatus legS = GetPartStatus(PartType::LegLeft, cData.legID);

    PlayerComponent pComp;
    pComp.maxHp = headS.hp + bodyS.hp + waistS.hp + armLS.hp + armRS.hp + legS.hp * 2.0f;
    pComp.currentHp = pComp.maxHp;
    pComp.totalAttack = headS.attack + bodyS.attack + waistS.attack + armLS.attack + armRS.attack + legS.attack * 2.0f;
    pComp.totalDefense = headS.defense + bodyS.defense + waistS.defense + armLS.defense + armRS.defense + legS.defense * 2.0f;
    pComp.totalWeight = headS.weight + bodyS.weight + waistS.weight + armLS.weight + armRS.weight + legS.weight * 2.0f;

    pComp.moveSpeed = 4.0f + legS.speed - (pComp.totalWeight * 0.04f);
    if (pComp.moveSpeed < 2.0f) pComp.moveSpeed = 2.0f;
    pComp.dashSpeed = pComp.moveSpeed * 1.6f;
    pComp.acceleration = 25.0f - (pComp.totalWeight * 0.15f);
    if (pComp.acceleration < 5.0f) pComp.acceleration = 5.0f;
    pComp.deceleration = 20.0f - (pComp.totalWeight * 0.2f);
    if (pComp.deceleration < 3.0f) pComp.deceleration = 3.0f;
    pComp.turnSpeed = 15.0f - (pComp.totalWeight * 0.1f);
    if (pComp.turnSpeed < 3.0f) pComp.turnSpeed = 3.0f;
    pComp.jumpPower = 7.0f - (pComp.totalWeight * 0.03f);
    if (pComp.jumpPower < 3.0f) pComp.jumpPower = 3.0f;

    pComp.isGrounded = params.isGrounded;
    pComp.type = (PlayerType)cData.bodyID;

    if (isEnemy) {
        world->AddComponent<EnemyComponent>(playerCore, EnemyComponent{});
        world->AddComponent<StatusComponent>(playerCore, StatusComponent{ .hp = (int)pComp.maxHp, .maxHp = (int)pComp.maxHp, .attackPower = (int)pComp.totalAttack });
        world->AddComponent<PhysicsComponent>(playerCore, PhysicsComponent{ .velocity = {0,0,0}, .useGravity = true });
        world->AddComponent<ColliderComponent>(playerCore);
        world->GetComponent<ColliderComponent>(playerCore).SetCapsule(0.5f, 2.0f);
    }
    else {
        world->AddComponent<PlayerComponent>(playerCore, pComp);
        world->AddComponent<StatusComponent>(playerCore, StatusComponent{ .hp = (int)pComp.maxHp, .maxHp = (int)pComp.maxHp, .attackPower = (int)pComp.totalAttack });
        world->AddComponent<PhysicsComponent>(playerCore, PhysicsComponent{ .velocity = {0,0,0}, .useGravity = true });
        world->AddComponent<ColliderComponent>(playerCore);
        world->GetComponent<ColliderComponent>(playerCore).SetCapsule(0.5f, 2.0f);
    }

    const float HEAD_Y = 1.7f;
    const float SHOULDER_X = 0.65f;
    const float SHOULDER_Y = 1.2f;
    const float BODY_Y = 1.0f;
    const float WAIST_Y = 0.55f;
    const float HIP_X = 0.35f;
    const float HIP_Y = 0.1f;

    auto BuildFrame = [&](PartType pType, EntityID parentEnt, DirectX::XMFLOAT3 localOffset) -> EntityID {
        EntityID pivot = world->CreateEntity().Build();
        world->AddComponent<TransformComponent>(pivot, TransformComponent{ .position = localOffset, .scale = {1.0f, 1.0f, 1.0f} });

        if (isEnemy) world->AddComponent<EnemyPartComponent>(pivot, EnemyPartComponent{ .parentID = (int)parentEnt, .partType = pType, .partModelID = -1, .baseOffset = localOffset });
        else world->AddComponent<PlayerPartComponent>(pivot, PlayerPartComponent{ .parentID = (int)parentEnt, .partType = pType, .partModelID = -1, .baseOffset = localOffset });

        EntityID visual = world->CreateEntity().Build();
        world->AddComponent<TransformComponent>(visual, TransformComponent{ .position = {0.0f, 0.0f, 0.0f}, .scale = {0.08f, 0.08f, 0.08f} });
        world->AddComponent<MeshComponent>(visual);
        AttachMeshAndCollider(visual, world, ShapeType::CYLINDER, { 0.2f, 0.2f, 0.2f, 1.0f }, ColliderType::Type_None, 0.0f, 0.0f, 0.0f);

        if (isEnemy) world->AddComponent<EnemyPartComponent>(visual, EnemyPartComponent{ .parentID = (int)pivot, .partType = pType, .partModelID = -2, .baseOffset = {0,0,0} });
        else world->AddComponent<PlayerPartComponent>(visual, PlayerPartComponent{ .parentID = (int)pivot, .partType = pType, .partModelID = -2, .baseOffset = {0,0,0} });

        return pivot;
        };

    EntityID frameWaist = BuildFrame(PartType::Waist, playerCore, { 0.0f, WAIST_Y, 0.0f });
    EntityID frameBody = BuildFrame(PartType::Body, frameWaist, { 0.0f, BODY_Y - WAIST_Y, 0.0f });
    EntityID frameHead = BuildFrame(PartType::Head, frameBody, { 0.0f, HEAD_Y - 1.2f, 0.0f });
    EntityID frameShL = BuildFrame(PartType::ShoulderLeft, frameBody, { -SHOULDER_X, SHOULDER_Y - BODY_Y, 0.0f });
    EntityID frameShR = BuildFrame(PartType::ShoulderRight, frameBody, { SHOULDER_X, SHOULDER_Y - BODY_Y, 0.0f });
    EntityID frameArmL = BuildFrame(PartType::ArmLeft, frameShL, { 0.0f, -0.4f, 0.0f });
    EntityID frameArmR = BuildFrame(PartType::ArmRight, frameShR, { 0.0f, -0.4f, 0.0f });
    EntityID frameLegL = BuildFrame(PartType::LegLeft, frameWaist, { -HIP_X, HIP_Y - WAIST_Y, 0.0f });
    EntityID frameLegR = BuildFrame(PartType::LegRight, frameWaist, { HIP_X, HIP_Y - WAIST_Y, 0.0f });

    auto BuildPart = [&](PartType pType, int modelID, DirectX::XMFLOAT3 absOffset, DirectX::XMFLOAT3 rot, DirectX::XMFLOAT3 scale, DirectX::XMFLOAT4 color, ShapeType shape, PartStatus status) {
        EntityID parentFrame = playerCore;
        DirectX::XMFLOAT3 frameAbsPos = { 0.0f, 0.0f, 0.0f };

        if (pType == PartType::Waist) { parentFrame = frameWaist; frameAbsPos = { 0.0f, WAIST_Y, 0.0f }; }
        else if (pType == PartType::Body) { parentFrame = frameBody; frameAbsPos = { 0.0f, BODY_Y, 0.0f }; }
        else if (pType == PartType::Head) { parentFrame = frameHead; frameAbsPos = { 0.0f, HEAD_Y, 0.0f }; }
        else if (pType == PartType::ShoulderLeft) { parentFrame = frameShL; frameAbsPos = { -SHOULDER_X, SHOULDER_Y, 0.0f }; }
        else if (pType == PartType::ShoulderRight) { parentFrame = frameShR; frameAbsPos = { SHOULDER_X, SHOULDER_Y, 0.0f }; }
        else if (pType == PartType::ArmLeft) { parentFrame = frameArmL; frameAbsPos = { -SHOULDER_X, SHOULDER_Y - 0.4f, 0.0f }; }
        else if (pType == PartType::ArmRight) { parentFrame = frameArmR; frameAbsPos = { SHOULDER_X, SHOULDER_Y - 0.4f, 0.0f }; }
        else if (pType == PartType::LegLeft) { parentFrame = frameLegL; frameAbsPos = { -HIP_X, HIP_Y, 0.0f }; }
        else if (pType == PartType::LegRight) { parentFrame = frameLegR; frameAbsPos = { HIP_X, HIP_Y, 0.0f }; }

        DirectX::XMFLOAT3 localOffset = { absOffset.x - frameAbsPos.x, absOffset.y - frameAbsPos.y, absOffset.z - frameAbsPos.z };

        EntityID armor = world->CreateEntity().Build();
        world->AddComponent<TransformComponent>(armor, TransformComponent{ .position = localOffset, .rotation = rot, .scale = scale });
        world->AddComponent<MeshComponent>(armor);
        AttachMeshAndCollider(armor, world, shape, color, ColliderType::Type_None, 0.0f, 0.0f, 0.0f);

        if (isEnemy) {
            world->AddComponent<EnemyPartComponent>(armor, EnemyPartComponent{ .parentID = (int)parentFrame, .partType = pType, .partModelID = modelID, .baseOffset = localOffset, .baseRotation = rot });
        }
        else {
            world->AddComponent<PlayerPartComponent>(armor, PlayerPartComponent{ .parentID = (int)parentFrame, .partType = pType, .partModelID = modelID, .baseOffset = localOffset, .baseRotation = rot, .status = status });
        }
        };
    DirectX::XMFLOAT4 cA_M = { 0.8f, 0.85f, 0.9f, 1.0f }; DirectX::XMFLOAT4 cA_S = { 0.2f, 0.3f,  0.5f, 1.0f }; DirectX::XMFLOAT4 cA_G = { 0.0f, 1.0f,  0.5f, 1.0f }; DirectX::XMFLOAT4 cA_F = { 0.3f, 0.3f,  0.3f, 1.0f };
    DirectX::XMFLOAT4 cB_M = { 0.4f, 0.5f,  0.3f, 1.0f }; DirectX::XMFLOAT4 cB_S = { 0.2f, 0.2f,  0.2f, 1.0f }; DirectX::XMFLOAT4 cB_F = { 0.15f,0.15f, 0.15f,1.0f };
    DirectX::XMFLOAT4 cC_M = { 0.15f,0.15f, 0.15f,1.0f }; DirectX::XMFLOAT4 cC_S = { 0.5f, 0.0f,  0.8f, 1.0f }; DirectX::XMFLOAT4 cC_F = { 0.4f, 0.4f,  0.4f, 1.0f };
    DirectX::XMFLOAT4 cD_M = { 0.1f, 0.15f, 0.3f, 1.0f }; DirectX::XMFLOAT4 cD_S = { 0.7f, 0.1f,  0.1f, 1.0f }; DirectX::XMFLOAT4 cD_F = { 0.2f, 0.2f,  0.25f,1.0f };
    DirectX::XMFLOAT4 cE_M = { 0.7f, 0.4f,  0.1f, 1.0f }; DirectX::XMFLOAT4 cE_S = { 0.3f, 0.3f,  0.3f, 1.0f }; DirectX::XMFLOAT4 cE_G = { 1.0f, 0.5f,  0.0f, 1.0f }; DirectX::XMFLOAT4 cE_F = { 0.2f, 0.2f,  0.2f, 1.0f };

    if (isEnemy) {
        DirectX::XMFLOAT4 eMain = { 0.2f, 0.2f, 0.25f, 1.0f };
        DirectX::XMFLOAT4 eSub = { 0.1f, 0.1f, 0.1f, 1.0f };
        DirectX::XMFLOAT4 eGlow1 = { 1.0f, 0.0f, 0.2f, 1.0f };
        DirectX::XMFLOAT4 eGlow2 = { 0.8f, 0.0f, 1.0f, 1.0f };
        DirectX::XMFLOAT4 eGlow = (cData.headID == 2) ? eGlow2 : eGlow1;

        cA_M = eMain; cA_S = eSub; cA_G = eGlow; cA_F = eSub;
        cB_M = eMain; cB_S = eSub; cB_F = eSub;
        cC_M = eMain; cC_S = eGlow; cC_F = eSub;
        cD_M = eMain; cD_S = eSub; cD_F = eSub;
        cE_M = eMain; cE_S = eSub; cE_G = eGlow; cE_F = eSub;
    }

    //“ª•”
    if (cData.headID == 0) {
        BuildPart(PartType::Head, 0, { 0, HEAD_Y - 0.05f, 0 }, { 0,0,0 }, { 0.15f, 0.15f, 0.15f }, cA_F, ShapeType::CYLINDER, headS);
        BuildPart(PartType::Head, 0, { 0, HEAD_Y + 0.05f, 0 }, { 0,0,0 }, { 0.25f, 0.25f, 0.25f }, cA_M, ShapeType::CUBE, headS);
        BuildPart(PartType::Head, 0, { 0, HEAD_Y + 0.05f, 0.15f }, { 0,0,0 }, { 0.18f, 0.08f, 0.05f }, cA_G, ShapeType::CUBE, headS);
        BuildPart(PartType::Head, 0, { 0, HEAD_Y + 0.12f, 0.15f }, { 0,0,0 }, { 0.12f, 0.04f, 0.05f }, cA_S, ShapeType::CUBE, headS);
        BuildPart(PartType::Head, 0, { 0, HEAD_Y + 0.15f, -0.05f }, { -0.2f,0,0 }, { 0.08f, 0.2f, 0.08f }, cA_S, ShapeType::WEDGE, headS);
        BuildPart(PartType::Head, 0, { -0.15f, HEAD_Y + 0.05f, 0 }, { 0,0,DirectX::XM_PIDIV2 }, { 0.08f, 0.1f, 0.1f }, cA_F, ShapeType::CYLINDER, headS);
        BuildPart(PartType::Head, 0, { 0.15f, HEAD_Y + 0.05f, 0 }, { 0,0,DirectX::XM_PIDIV2 }, { 0.08f, 0.1f, 0.1f }, cA_F, ShapeType::CYLINDER, headS);
        BuildPart(PartType::Head, 0, { 0, HEAD_Y - 0.02f, 0.12f }, { 0,0,0 }, { 0.15f, 0.05f, 0.1f }, cA_S, ShapeType::WEDGE, headS);
    }
    else if (cData.headID == 1) {
        BuildPart(PartType::Head, 1, { 0, HEAD_Y - 0.05f, 0 }, { 0,0,0 }, { 0.25f, 0.15f, 0.25f }, cB_F, ShapeType::CYLINDER, headS);
        BuildPart(PartType::Head, 1, { 0, HEAD_Y + 0.05f, 0 }, { 0,0,0 }, { 0.35f, 0.25f, 0.35f }, cB_M, ShapeType::CUBE, headS);
        BuildPart(PartType::Head, 1, { 0, HEAD_Y + 0.05f, 0.18f }, { 0,0,0 }, { 0.35f, 0.15f, 0.05f }, cB_S, ShapeType::CUBE, headS);
        BuildPart(PartType::Head, 1, { 0, HEAD_Y + 0.12f, 0.18f }, { 0,0,0 }, { 0.15f, 0.05f, 0.05f }, cB_F, ShapeType::CUBE, headS);
        BuildPart(PartType::Head, 1, { 0, HEAD_Y + 0.2f, -0.1f }, { 0,0,0 }, { 0.35f, 0.05f, 0.35f }, cB_M, ShapeType::CUBE, headS);
        BuildPart(PartType::Head, 1, { -0.2f, HEAD_Y + 0.05f, 0.1f }, { 0,0,0 }, { 0.05f, 0.2f, 0.15f }, cB_S, ShapeType::CUBE, headS);
        BuildPart(PartType::Head, 1, { 0.2f, HEAD_Y + 0.05f, 0.1f }, { 0,0,0 }, { 0.05f, 0.2f, 0.15f }, cB_S, ShapeType::CUBE, headS);
    }
    else if (cData.headID == 2) {
        BuildPart(PartType::Head, 2, { 0, HEAD_Y - 0.05f, 0 }, { 0,0,0 }, { 0.12f, 0.15f, 0.12f }, cC_F, ShapeType::CYLINDER, headS);
        BuildPart(PartType::Head, 2, { 0, HEAD_Y + 0.05f, 0.05f }, { 0.1f,0,0 }, { 0.2f, 0.3f, 0.3f }, cC_M, ShapeType::WEDGE, headS);
        BuildPart(PartType::Head, 2, { 0, HEAD_Y + 0.15f, -0.1f }, { -0.3f,0,0 }, { 0.1f, 0.15f, 0.2f }, cC_S, ShapeType::DOUBLE_PYRAMID, headS);
        BuildPart(PartType::Head, 2, { 0, HEAD_Y + 0.05f, 0.22f }, { 0,0,0 }, { 0.08f, 0.08f, 0.08f }, cC_F, ShapeType::SPHERE, headS);
        BuildPart(PartType::Head, 2, { 0, HEAD_Y + 0.15f, 0.15f }, { 0,0,0 }, { 0.15f, 0.05f, 0.1f }, cC_S, ShapeType::CUBE, headS);
        BuildPart(PartType::Head, 2, { -0.1f, HEAD_Y + 0.1f, 0.15f }, { 0,0,0 }, { 0.02f, 0.1f, 0.1f }, cC_S, ShapeType::CUBE, headS);
        BuildPart(PartType::Head, 2, { 0.1f, HEAD_Y + 0.1f, 0.15f }, { 0,0,0 }, { 0.02f, 0.1f, 0.1f }, cC_S, ShapeType::CUBE, headS);
    }
    else if (cData.headID == 3) {
        BuildPart(PartType::Head, 3, { 0, HEAD_Y - 0.08f, 0 }, { 0,0,0 }, { 0.3f, 0.15f, 0.3f }, cD_F, ShapeType::CYLINDER, headS);
        BuildPart(PartType::Head, 3, { 0, HEAD_Y, 0 }, { 0,0,0 }, { 0.4f, 0.25f, 0.4f }, cD_M, ShapeType::HEXAGONAL_PRISM, headS);
        BuildPart(PartType::Head, 3, { 0, HEAD_Y + 0.15f, 0 }, { 0,0,0 }, { 0.35f, 0.05f, 0.35f }, cD_S, ShapeType::CUBE, headS);
        BuildPart(PartType::Head, 3, { 0, HEAD_Y, 0.22f }, { 0,0,0 }, { 0.25f, 0.08f, 0.05f }, cD_S, ShapeType::CUBE, headS);
        BuildPart(PartType::Head, 3, { 0, HEAD_Y - 0.08f, 0.22f }, { 0,0,0 }, { 0.3f, 0.1f, 0.1f }, cD_M, ShapeType::CUBE, headS);
        BuildPart(PartType::Head, 3, { -0.22f, HEAD_Y, 0 }, { 0,0,0 }, { 0.08f, 0.2f, 0.2f }, cD_F, ShapeType::CYLINDER, headS);
        BuildPart(PartType::Head, 3, { 0.22f, HEAD_Y, 0 }, { 0,0,0 }, { 0.08f, 0.2f, 0.2f }, cD_F, ShapeType::CYLINDER, headS);
    }
    else if (cData.headID == 4) {
        BuildPart(PartType::Head, 4, { 0, HEAD_Y - 0.05f, 0 }, { 0,0,0 }, { 0.18f, 0.15f, 0.18f }, cE_F, ShapeType::CYLINDER, headS);
        BuildPart(PartType::Head, 4, { 0, HEAD_Y + 0.05f, 0 }, { 0,0,0 }, { 0.25f, 0.25f, 0.25f }, cE_M, ShapeType::CUBE, headS);
        BuildPart(PartType::Head, 4, { 0, HEAD_Y + 0.15f, 0.15f }, { -0.3f,0,0 }, { 0.08f, 0.35f, 0.2f }, cE_S, ShapeType::DOUBLE_PYRAMID, headS);
        BuildPart(PartType::Head, 4, { 0, HEAD_Y + 0.02f, 0.15f }, { 0,0,0 }, { 0.12f, 0.12f, 0.08f }, cE_G, ShapeType::SPHERE, headS);
        BuildPart(PartType::Head, 4, { -0.15f, HEAD_Y + 0.05f, -0.1f }, { 0.2f,-0.2f,0 }, { 0.05f, 0.2f, 0.2f }, cE_M, ShapeType::WEDGE, headS);
        BuildPart(PartType::Head, 4, { 0.15f, HEAD_Y + 0.05f, -0.1f }, { 0.2f,0.2f,0 }, { 0.05f, 0.2f, 0.2f }, cE_M, ShapeType::WEDGE, headS);
        BuildPart(PartType::Head, 4, { 0, HEAD_Y - 0.05f, 0.15f }, { 0.2f,0,0 }, { 0.1f, 0.15f, 0.1f }, cE_F, ShapeType::WEDGE, headS);
    }

    //“·‘Ì
    if (cData.bodyID == 0) {
        BuildPart(PartType::Body, 0, { 0, BODY_Y - 0.05f, 0 }, { 0,0,0 }, { 0.35f, 0.45f, 0.25f }, cA_F, ShapeType::CYLINDER, bodyS);
        BuildPart(PartType::Body, 0, { 0, BODY_Y + 0.15f, 0.05f }, { -0.1f,0,0 }, { 0.5f, 0.35f, 0.35f }, cA_S, ShapeType::CUBE, bodyS);
        BuildPart(PartType::Body, 0, { 0, BODY_Y - 0.05f, 0.15f }, { 0.1f,0,0 }, { 0.4f, 0.25f, 0.15f }, cA_M, ShapeType::WEDGE, bodyS);
        BuildPart(PartType::Body, 0, { 0, BODY_Y + 0.25f, 0.15f }, { 0,0,0 }, { 0.2f, 0.1f, 0.1f }, cA_G, ShapeType::CUBE, bodyS);
        BuildPart(PartType::Body, 0, { 0, BODY_Y + 0.35f, 0 }, { 0,0,0 }, { 0.3f, 0.1f, 0.25f }, cA_M, ShapeType::CUBE, bodyS);
        BuildPart(PartType::Body, 0, { -0.15f, BODY_Y + 0.15f, 0.32f }, { 0,0,0 }, { 0.1f, 0.15f, 0.05f }, cA_F, ShapeType::CUBE, bodyS);
        BuildPart(PartType::Body, 0, { 0.15f, BODY_Y + 0.15f, 0.32f }, { 0,0,0 }, { 0.1f, 0.15f, 0.05f }, cA_F, ShapeType::CUBE, bodyS);
        BuildPart(PartType::Body, 0, { 0, BODY_Y + 0.15f, -0.25f }, { 0,0,0 }, { 0.35f, 0.45f, 0.25f }, cA_F, ShapeType::CUBE, bodyS);
        BuildPart(PartType::Body, 0, { 0, BODY_Y, -0.3f }, { -0.2f,0,0 }, { 0.2f, 0.15f, 0.1f }, cA_M, ShapeType::WEDGE, bodyS);
    }
    else if (cData.bodyID == 1) {
        BuildPart(PartType::Body, 1, { 0, BODY_Y + 0.1f, 0 }, { 0,0,0 }, { 0.65f, 0.7f, 0.5f }, cB_M, ShapeType::CUBE, bodyS);
        BuildPart(PartType::Body, 1, { 0, BODY_Y + 0.2f, 0.28f }, { 0,0,0 }, { 0.45f, 0.35f, 0.25f }, cB_S, ShapeType::TRUNCATED_CONE, bodyS);
        BuildPart(PartType::Body, 1, { 0, BODY_Y - 0.05f, 0.28f }, { 0,0,0 }, { 0.55f, 0.2f, 0.15f }, cB_F, ShapeType::CUBE, bodyS);
        BuildPart(PartType::Body, 1, { -0.2f, BODY_Y + 0.2f, 0.4f }, { 0,0,0 }, { 0.1f, 0.15f, 0.05f }, cB_F, ShapeType::CUBE, bodyS);
        BuildPart(PartType::Body, 1, { 0.2f, BODY_Y + 0.2f, 0.4f }, { 0,0,0 }, { 0.1f, 0.15f, 0.05f }, cB_F, ShapeType::CUBE, bodyS);
        BuildPart(PartType::Body, 1, { 0, BODY_Y + 0.1f, -0.35f }, { 0,0,0 }, { 0.55f, 0.5f, 0.25f }, cB_F, ShapeType::HEXAGONAL_PRISM, bodyS);
        BuildPart(PartType::Body, 1, { 0, BODY_Y + 0.45f, 0 }, { 0,0,0 }, { 0.7f, 0.15f, 0.5f }, cB_M, ShapeType::CUBE, bodyS);
        BuildPart(PartType::Body, 1, { 0, BODY_Y - 0.1f, 0 }, { 0,0,0 }, { 0.4f, 0.3f, 0.4f }, cB_F, ShapeType::CYLINDER, bodyS);
    }
    else if (cData.bodyID == 2) {
        BuildPart(PartType::Body, 2, { 0, BODY_Y + 0.1f, 0 }, { 0.1f,0,0 }, { 0.4f, 0.65f, 0.4f }, cC_M, ShapeType::DOUBLE_PYRAMID, bodyS);
        BuildPart(PartType::Body, 2, { 0, BODY_Y + 0.25f, 0.2f }, { -0.2f,0,0 }, { 0.35f, 0.3f, 0.25f }, cC_S, ShapeType::WEDGE, bodyS);
        BuildPart(PartType::Body, 2, { 0, BODY_Y, 0.25f }, { 0,0,0 }, { 0.25f, 0.2f, 0.15f }, cC_M, ShapeType::HEXAGONAL_PRISM, bodyS);
        BuildPart(PartType::Body, 2, { 0, BODY_Y + 0.35f, 0.1f }, { -0.1f,0,0 }, { 0.2f, 0.1f, 0.15f }, cC_F, ShapeType::CUBE, bodyS);
        BuildPart(PartType::Body, 2, { -0.2f, BODY_Y + 0.1f, -0.2f }, { 0.2f,0,0 }, { 0.15f, 0.4f, 0.2f }, cC_F, ShapeType::CYLINDER, bodyS);
        BuildPart(PartType::Body, 2, { 0.2f, BODY_Y + 0.1f, -0.2f }, { 0.2f,0,0 }, { 0.15f, 0.4f, 0.2f }, cC_F, ShapeType::CYLINDER, bodyS);
        BuildPart(PartType::Body, 2, { -0.2f, BODY_Y - 0.1f, -0.25f }, { 0.4f,0,0 }, { 0.1f, 0.2f, 0.1f }, cC_S, ShapeType::DOUBLE_PYRAMID, bodyS);
        BuildPart(PartType::Body, 2, { 0.2f, BODY_Y - 0.1f, -0.25f }, { 0.4f,0,0 }, { 0.1f, 0.2f, 0.1f }, cC_S, ShapeType::DOUBLE_PYRAMID, bodyS);
    }
    else if (cData.bodyID == 3) {
        BuildPart(PartType::Body, 3, { 0, BODY_Y + 0.1f, 0 }, { 0,0,0 }, { 0.75f, 0.7f, 0.55f }, cD_M, ShapeType::HEXAGONAL_PRISM, bodyS);
        BuildPart(PartType::Body, 3, { 0, BODY_Y + 0.15f, 0.32f }, { 0,0,0 }, { 0.55f, 0.4f, 0.15f }, cD_S, ShapeType::CUBE, bodyS);
        BuildPart(PartType::Body, 3, { 0, BODY_Y - 0.05f, 0.32f }, { 0,0,0 }, { 0.4f, 0.2f, 0.1f }, cD_M, ShapeType::CUBE, bodyS);
        BuildPart(PartType::Body, 3, { 0, BODY_Y + 0.35f, 0.15f }, { 0,0,0 }, { 0.65f, 0.15f, 0.2f }, cD_F, ShapeType::CUBE, bodyS);
        BuildPart(PartType::Body, 3, { -0.32f, BODY_Y, 0.3f }, { 0,0,0 }, { 0.15f, 0.4f, 0.15f }, cD_F, ShapeType::CYLINDER, bodyS);
        BuildPart(PartType::Body, 3, { 0.32f, BODY_Y, 0.3f }, { 0,0,0 }, { 0.15f, 0.4f, 0.15f }, cD_F, ShapeType::CYLINDER, bodyS);
        BuildPart(PartType::Body, 3, { 0, BODY_Y + 0.1f, -0.3f }, { 0,0,0 }, { 0.6f, 0.5f, 0.15f }, cD_F, ShapeType::CUBE, bodyS);
    }
    else if (cData.bodyID == 4) {
        BuildPart(PartType::Body, 4, { 0, BODY_Y + 0.05f, 0.05f }, { -0.1f,0,0 }, { 0.45f, 0.65f, 0.45f }, cE_M, ShapeType::WEDGE, bodyS);
        BuildPart(PartType::Body, 4, { 0, BODY_Y + 0.1f, -0.15f }, { 0,0,0 }, { 0.3f, 0.3f, 0.3f }, cE_G, ShapeType::SPHERE, bodyS);
        BuildPart(PartType::Body, 4, { 0, BODY_Y + 0.2f, 0.3f }, { 0,0,0 }, { 0.2f, 0.5f, 0.2f }, cE_S, ShapeType::DOUBLE_PYRAMID, bodyS);
        BuildPart(PartType::Body, 4, { -0.25f, BODY_Y + 0.2f, 0.2f }, { -0.2f,0,0.2f }, { 0.15f, 0.4f, 0.15f }, cE_S, ShapeType::WEDGE, bodyS);
        BuildPart(PartType::Body, 4, { 0.25f, BODY_Y + 0.2f, 0.2f }, { -0.2f,0,-0.2f }, { 0.15f, 0.4f, 0.15f }, cE_S, ShapeType::WEDGE, bodyS);
        BuildPart(PartType::Body, 4, { 0, BODY_Y - 0.1f, 0.15f }, { 0.2f,0,0 }, { 0.3f, 0.2f, 0.2f }, cE_F, ShapeType::CUBE, bodyS);
        BuildPart(PartType::Body, 4, { 0, BODY_Y + 0.35f, 0 }, { -0.2f,0,0 }, { 0.4f, 0.15f, 0.3f }, cE_M, ShapeType::CUBE, bodyS);
    }

    //˜•”
    if (cData.waistID == 0) {
        BuildPart(PartType::Waist, 0, { 0, WAIST_Y, 0 }, { 0,0,0 }, { 0.4f, 0.3f, 0.35f }, cA_M, ShapeType::CUBE, waistS);
        BuildPart(PartType::Waist, 0, { 0, WAIST_Y - 0.1f, 0.25f }, { -0.1f,0,0 }, { 0.3f, 0.35f, 0.08f }, cA_S, ShapeType::WEDGE, waistS);
        BuildPart(PartType::Waist, 0, { 0, WAIST_Y - 0.1f, -0.25f }, { 0.1f,0,0 }, { 0.4f, 0.3f, 0.08f }, cA_M, ShapeType::WEDGE, waistS);
        BuildPart(PartType::Waist, 0, { -0.28f, WAIST_Y - 0.1f, 0 }, { 0,0,0.1f }, { 0.08f, 0.4f, 0.28f }, cA_F, ShapeType::CUBE, waistS);
        BuildPart(PartType::Waist, 0, { 0.28f, WAIST_Y - 0.1f, 0 }, { 0,0,-0.1f }, { 0.08f, 0.4f, 0.28f }, cA_F, ShapeType::CUBE, waistS);
        BuildPart(PartType::Waist, 0, { 0, WAIST_Y - 0.15f, 0 }, { 0,0,0 }, { 0.2f, 0.2f, 0.2f }, cA_F, ShapeType::CYLINDER, waistS);
    }
    else if (cData.waistID == 1) {
        BuildPart(PartType::Waist, 1, { 0, WAIST_Y, 0 }, { 0,0,0 }, { 0.65f, 0.4f, 0.5f }, cB_M, ShapeType::HEXAGONAL_PRISM, waistS);
        BuildPart(PartType::Waist, 1, { 0, WAIST_Y - 0.05f, 0.3f }, { 0,0,0 }, { 0.45f, 0.4f, 0.15f }, cB_S, ShapeType::CUBE, waistS);
        BuildPart(PartType::Waist, 1, { 0, WAIST_Y - 0.05f, -0.3f }, { 0,0,0 }, { 0.55f, 0.4f, 0.15f }, cB_F, ShapeType::CUBE, waistS);
        BuildPart(PartType::Waist, 1, { -0.38f, WAIST_Y - 0.1f, 0 }, { 0,0,0 }, { 0.18f, 0.45f, 0.35f }, cB_M, ShapeType::CYLINDER, waistS);
        BuildPart(PartType::Waist, 1, { 0.38f, WAIST_Y - 0.1f, 0 }, { 0,0,0 }, { 0.18f, 0.45f, 0.35f }, cB_M, ShapeType::CYLINDER, waistS);
        BuildPart(PartType::Waist, 1, { 0, WAIST_Y + 0.15f, 0.28f }, { 0,0,0 }, { 0.25f, 0.15f, 0.1f }, cB_F, ShapeType::CUBE, waistS);
    }
    else if (cData.waistID == 2) {
        BuildPart(PartType::Waist, 2, { 0, WAIST_Y, 0 }, { 0,0,0 }, { 0.4f, 0.25f, 0.35f }, cC_M, ShapeType::TRUNCATED_CONE, waistS);
        BuildPart(PartType::Waist, 2, { 0, WAIST_Y - 0.1f, 0.22f }, { -0.2f,0,0 }, { 0.2f, 0.3f, 0.08f }, cC_S, ShapeType::WEDGE, waistS);
        BuildPart(PartType::Waist, 2, { 0, WAIST_Y - 0.2f, -0.25f }, { 0.3f,0,0 }, { 0.25f, 0.5f, 0.1f }, cC_S, ShapeType::WEDGE, waistS);
        BuildPart(PartType::Waist, 2, { -0.25f, WAIST_Y - 0.05f, 0 }, { 0,0,DirectX::XM_PIDIV2 }, { 0.1f, 0.2f, 0.2f }, cC_F, ShapeType::DOUBLE_PYRAMID, waistS);
        BuildPart(PartType::Waist, 2, { 0.25f, WAIST_Y - 0.05f, 0 }, { 0,0,DirectX::XM_PIDIV2 }, { 0.1f, 0.2f, 0.2f }, cC_F, ShapeType::DOUBLE_PYRAMID, waistS);
        BuildPart(PartType::Waist, 2, { 0, WAIST_Y - 0.15f, 0 }, { 0,0,0 }, { 0.15f, 0.2f, 0.15f }, cC_F, ShapeType::CYLINDER, waistS);
    }
    else if (cData.waistID == 3) {
        BuildPart(PartType::Waist, 3, { 0, WAIST_Y, 0 }, { 0,0,0 }, { 0.65f, 0.35f, 0.5f }, cD_M, ShapeType::CUBE, waistS);
        BuildPart(PartType::Waist, 3, { 0, WAIST_Y - 0.25f, 0 }, { 0,0,0 }, { 0.95f, 0.5f, 0.8f }, cD_S, ShapeType::TRUNCATED_CONE, waistS);
        BuildPart(PartType::Waist, 3, { 0, WAIST_Y - 0.3f, 0.42f }, { 0,0,0 }, { 0.4f, 0.25f, 0.08f }, cD_F, ShapeType::CUBE, waistS);
        BuildPart(PartType::Waist, 3, { -0.45f, WAIST_Y - 0.3f, 0 }, { 0,0,0 }, { 0.1f, 0.35f, 0.3f }, cD_F, ShapeType::CUBE, waistS);
        BuildPart(PartType::Waist, 3, { 0.45f, WAIST_Y - 0.3f, 0 }, { 0,0,0 }, { 0.1f, 0.35f, 0.3f }, cD_F, ShapeType::CUBE, waistS);
    }
    else if (cData.waistID == 4) {
        BuildPart(PartType::Waist, 4, { 0, WAIST_Y, 0 }, { 0,0,0 }, { 0.45f, 0.3f, 0.35f }, cE_M, ShapeType::CUBE, waistS);
        BuildPart(PartType::Waist, 4, { 0, WAIST_Y - 0.15f, 0.22f }, { -0.2f,0,0 }, { 0.25f, 0.45f, 0.12f }, cE_G, ShapeType::WEDGE, waistS);
        BuildPart(PartType::Waist, 4, { -0.28f, WAIST_Y - 0.2f, 0.15f }, { -0.3f,0,0.4f }, { 0.15f, 0.45f, 0.15f }, cE_S, ShapeType::DOUBLE_PYRAMID, waistS);
        BuildPart(PartType::Waist, 4, { 0.28f, WAIST_Y - 0.2f, 0.15f }, { -0.3f,0,-0.4f }, { 0.15f, 0.45f, 0.15f }, cE_S, ShapeType::DOUBLE_PYRAMID, waistS);
        BuildPart(PartType::Waist, 4, { 0, WAIST_Y - 0.15f, -0.2f }, { 0.2f,0,0 }, { 0.3f, 0.35f, 0.1f }, cE_F, ShapeType::WEDGE, waistS);
    }

    //˜r•”
    auto BuildArm = [&](PartType shoulderT, PartType armT, int modelID, float dirX, PartStatus status) {
        float sX = dirX * SHOULDER_X;
        float armY = SHOULDER_Y - 0.4f;

        if (modelID == 0) {
            BuildPart(shoulderT, 0, { sX - dirX * 0.1f, SHOULDER_Y, 0 }, { 0,0,DirectX::XM_PIDIV2 }, { 0.15f, 0.2f, 0.15f }, cA_F, ShapeType::CYLINDER, status);
            BuildPart(shoulderT, 0, { sX + dirX * 0.05f, SHOULDER_Y + 0.05f, 0 }, { 0,0,0 }, { 0.35f, 0.3f, 0.35f }, cA_M, ShapeType::CUBE, status);
            BuildPart(shoulderT, 0, { sX + dirX * 0.28f, SHOULDER_Y, 0 }, { 0,0,0 }, { 0.15f, 0.4f, 0.4f }, cA_S, ShapeType::WEDGE, status);
            BuildPart(shoulderT, 0, { sX + dirX * 0.05f, SHOULDER_Y + 0.2f, 0 }, { 0,0,0 }, { 0.2f, 0.05f, 0.2f }, cA_G, ShapeType::CUBE, status);
            BuildPart(armT, 0, { sX + dirX * 0.05f, armY + 0.15f, 0 }, { 0,0,0 }, { 0.18f, 0.3f, 0.18f }, cA_F, ShapeType::CYLINDER, status);
            BuildPart(armT, 0, { sX + dirX * 0.05f, armY - 0.25f, 0 }, { 0,0,0 }, { 0.22f, 0.5f, 0.22f }, cA_M, ShapeType::CUBE, status);
            BuildPart(armT, 0, { sX + dirX * 0.05f, armY - 0.25f, -0.12f }, { 0,0,0 }, { 0.25f, 0.35f, 0.08f }, cA_S, ShapeType::CUBE, status);
            BuildPart(armT, 0, { sX + dirX * 0.05f, armY - 0.55f, 0 }, { 0,0,0 }, { 0.15f, 0.15f, 0.15f }, cA_F, ShapeType::CUBE, status);
        }
        else if (modelID == 1) {
            BuildPart(shoulderT, 1, { sX, SHOULDER_Y, 0 }, { 0,0,0 }, { 0.5f, 0.5f, 0.5f }, cB_M, ShapeType::SPHERE, status);
            BuildPart(shoulderT, 1, { sX, SHOULDER_Y, 0 }, { 0,0,0 }, { 0.55f, 0.2f, 0.55f }, cB_F, ShapeType::TORUS, status);
            BuildPart(shoulderT, 1, { sX + dirX * 0.4f, SHOULDER_Y, 0 }, { 0,0,0 }, { 0.2f, 0.35f, 0.25f }, cB_S, ShapeType::CYLINDER, status);
            BuildPart(armT, 1, { sX, armY, 0 }, { 0,0,0 }, { 0.35f, 0.65f, 0.35f }, cB_F, ShapeType::CYLINDER, status);
            BuildPart(armT, 1, { sX, armY - 0.25f, 0.2f }, { 0,0,0 }, { 0.25f, 0.3f, 0.15f }, cB_S, ShapeType::CUBE, status);
            BuildPart(armT, 1, { sX, armY + 0.2f, 0 }, { 0,0,0 }, { 0.4f, 0.15f, 0.4f }, cB_M, ShapeType::TORUS, status);
            BuildPart(armT, 1, { sX, armY - 0.6f, 0 }, { 0,0,0 }, { 0.25f, 0.2f, 0.25f }, cB_M, ShapeType::CUBE, status);
        }
        else if (modelID == 2) {
            BuildPart(shoulderT, 2, { sX - dirX * 0.1f, SHOULDER_Y, 0 }, { 0,0,DirectX::XM_PIDIV2 }, { 0.12f, 0.15f, 0.12f }, cC_F, ShapeType::CYLINDER, status);
            BuildPart(shoulderT, 2, { sX + dirX * 0.05f, SHOULDER_Y, 0 }, { 0,0,dirX * -0.2f }, { 0.3f, 0.22f, 0.4f }, cC_M, ShapeType::WEDGE, status);
            BuildPart(shoulderT, 2, { sX + dirX * 0.05f, SHOULDER_Y + 0.18f, -0.12f }, { 0.3f,0,0 }, { 0.08f, 0.35f, 0.18f }, cC_S, ShapeType::DOUBLE_PYRAMID, status);
            BuildPart(armT, 2, { sX + dirX * 0.05f, armY, 0 }, { 0,0,0 }, { 0.18f, 0.65f, 0.18f }, cC_F, ShapeType::CYLINDER, status);
            BuildPart(armT, 2, { sX + dirX * 0.05f, armY - 0.15f, -0.12f }, { 0,0,0 }, { 0.06f, 0.35f, 0.06f }, cC_S, ShapeType::CYLINDER, status);
            BuildPart(armT, 2, { sX + dirX * 0.05f, armY - 0.15f, 0.12f }, { 0,0,0 }, { 0.06f, 0.35f, 0.06f }, cC_S, ShapeType::CYLINDER, status);
            BuildPart(armT, 2, { sX + dirX * 0.05f, armY - 0.55f, 0 }, { 0,0,0 }, { 0.12f, 0.15f, 0.12f }, cC_M, ShapeType::SPHERE, status);
        }
        else if (modelID == 3) {
            BuildPart(shoulderT, 3, { sX, SHOULDER_Y, 0 }, { 0,0,0 }, { 0.35f, 0.35f, 0.35f }, cD_F, ShapeType::SPHERE, status);
            BuildPart(shoulderT, 3, { sX + dirX * 0.18f, SHOULDER_Y + 0.15f, 0 }, { 0,0,0 }, { 0.55f, 0.5f, 0.55f }, cD_M, ShapeType::CUBE, status);
            BuildPart(shoulderT, 3, { sX + dirX * 0.5f, SHOULDER_Y + 0.1f, 0 }, { 0,0,0 }, { 0.18f, 0.6f, 0.6f }, cD_S, ShapeType::CUBE, status);
            BuildPart(shoulderT, 3, { sX + dirX * 0.18f, SHOULDER_Y + 0.4f, 0 }, { 0,0,0 }, { 0.3f, 0.1f, 0.3f }, cD_S, ShapeType::CUBE, status);
            BuildPart(armT, 3, { sX + dirX * 0.18f, armY, 0 }, { 0,0,0 }, { 0.32f, 0.6f, 0.32f }, cD_F, ShapeType::HEXAGONAL_PRISM, status);
            BuildPart(armT, 3, { sX + dirX * 0.18f, armY - 0.2f, 0.2f }, { 0,0,0 }, { 0.25f, 0.3f, 0.1f }, cD_M, ShapeType::CUBE, status);
            BuildPart(armT, 3, { sX + dirX * 0.18f, armY - 0.55f, 0 }, { 0,0,0 }, { 0.2f, 0.2f, 0.2f }, cD_F, ShapeType::CUBE, status);
        }
        else if (modelID == 4) {
            BuildPart(shoulderT, 4, { sX, SHOULDER_Y, 0 }, { 0,0,0 }, { 0.25f, 0.25f, 0.25f }, cE_F, ShapeType::SPHERE, status);
            BuildPart(shoulderT, 4, { sX + dirX * 0.15f, SHOULDER_Y, 0.05f }, { 0,0,0 }, { 0.3f, 0.5f, 0.5f }, cE_S, ShapeType::DOUBLE_PYRAMID, status);
            BuildPart(shoulderT, 4, { sX + dirX * 0.15f, SHOULDER_Y + 0.2f, 0.05f }, { 0,0,0 }, { 0.15f, 0.15f, 0.15f }, cE_G, ShapeType::SPHERE, status);
            BuildPart(armT, 4, { sX + dirX * 0.15f, armY + 0.15f, 0 }, { 0,0,0 }, { 0.18f, 0.35f, 0.18f }, cE_M, ShapeType::CYLINDER, status);
            BuildPart(armT, 4, { sX + dirX * 0.15f, armY - 0.25f, 0 }, { 0,0,0 }, { 0.22f, 0.45f, 0.22f }, cE_M, ShapeType::CYLINDER, status);
            BuildPart(armT, 4, { sX + dirX * 0.15f, armY - 0.35f, 0.22f }, { -0.2f,0,0 }, { 0.08f, 0.7f, 0.25f }, cE_F, ShapeType::WEDGE, status);
            BuildPart(armT, 4, { sX + dirX * 0.15f, armY - 0.55f, 0 }, { 0,0,0 }, { 0.15f, 0.2f, 0.15f }, cE_M, ShapeType::WEDGE, status);
        }
        };
    BuildArm(PartType::ShoulderLeft, PartType::ArmLeft, cData.armLeftID, -1.0f, armLS);
    BuildArm(PartType::ShoulderRight, PartType::ArmRight, cData.armRightID, 1.0f, armRS);

    //‹r•”
    auto BuildLeg = [&](PartType legT, int modelID, float dirX, PartStatus status) {
        float hX = dirX * HIP_X;
        float legY = HIP_Y - 0.55f;

        if (modelID == 0) {
            BuildPart(legT, 0, { hX - dirX * 0.05f, HIP_Y, 0 }, { 0,0,DirectX::XM_PIDIV2 }, { 0.18f, 0.25f, 0.18f }, cA_F, ShapeType::CYLINDER, status);
            BuildPart(legT, 0, { hX, HIP_Y - 0.25f, 0 }, { 0,0,0 }, { 0.32f, 0.45f, 0.32f }, cA_M, ShapeType::CUBE, status);
            BuildPart(legT, 0, { hX, HIP_Y - 0.45f, 0.18f }, { 0,0,0 }, { 0.25f, 0.25f, 0.1f }, cA_S, ShapeType::WEDGE, status);
            BuildPart(legT, 0, { hX, HIP_Y - 0.5f, 0.18f }, { 0.2f,0,0 }, { 0.15f, 0.15f, 0.12f }, cA_G, ShapeType::CUBE, status);
            BuildPart(legT, 0, { hX, HIP_Y - 0.7f, 0 }, { 0,0,0 }, { 0.28f, 0.5f, 0.28f }, cA_M, ShapeType::CUBE, status);
            BuildPart(legT, 0, { hX, HIP_Y - 0.7f, -0.15f }, { -0.1f,0,0 }, { 0.18f, 0.35f, 0.12f }, cA_F, ShapeType::WEDGE, status);
            BuildPart(legT, 0, { hX, HIP_Y - 0.95f, 0.05f }, { 0,0,0 }, { 0.32f, 0.15f, 0.4f }, cA_S, ShapeType::CUBE, status);
            BuildPart(legT, 0, { hX, HIP_Y - 0.95f, -0.18f }, { 0,0,0 }, { 0.28f, 0.15f, 0.25f }, cA_F, ShapeType::CUBE, status);
        }
        else if (modelID == 1) {
            BuildPart(legT, 1, { hX, HIP_Y, 0 }, { 0,0,0 }, { 0.3f, 0.3f, 0.3f }, cB_F, ShapeType::SPHERE, status);
            BuildPart(legT, 1, { hX, HIP_Y - 0.4f, 0 }, { 0,0,0 }, { 0.5f, 0.85f, 0.5f }, cB_M, ShapeType::HEXAGONAL_PRISM, status);
            BuildPart(legT, 1, { hX + dirX * 0.3f, HIP_Y - 0.4f, 0 }, { 0,0,0 }, { 0.18f, 0.45f, 0.3f }, cB_S, ShapeType::CUBE, status);
            BuildPart(legT, 1, { hX, HIP_Y - 0.3f, 0.3f }, { 0,0,0 }, { 0.3f, 0.3f, 0.15f }, cB_S, ShapeType::CUBE, status);
            BuildPart(legT, 1, { hX, HIP_Y - 0.6f, -0.28f }, { 0,0,0 }, { 0.35f, 0.35f, 0.15f }, cB_F, ShapeType::CUBE, status);
            BuildPart(legT, 1, { hX, HIP_Y - 0.9f, 0.12f }, { 0,0,0 }, { 0.55f, 0.25f, 0.6f }, cB_F, ShapeType::CUBE, status);
            BuildPart(legT, 1, { hX, HIP_Y - 0.9f, -0.2f }, { 0,0,0 }, { 0.45f, 0.25f, 0.3f }, cB_M, ShapeType::CUBE, status);
        }
        else if (modelID == 2) {
            BuildPart(legT, 2, { hX, HIP_Y, 0 }, { 0,0,DirectX::XM_PIDIV2 }, { 0.18f, 0.18f, 0.18f }, cC_F, ShapeType::CYLINDER, status);
            BuildPart(legT, 2, { hX, HIP_Y - 0.45f, 0.05f }, { -0.1f,0,0 }, { 0.22f, 0.9f, 0.22f }, cC_M, ShapeType::CYLINDER, status);
            BuildPart(legT, 2, { hX, HIP_Y - 0.35f, 0.22f }, { -0.2f,0,0 }, { 0.15f, 0.25f, 0.08f }, cC_S, ShapeType::WEDGE, status);
            BuildPart(legT, 2, { hX, HIP_Y - 0.6f, -0.18f }, { 0.2f,0,0 }, { 0.18f, 0.45f, 0.18f }, cC_S, ShapeType::DOUBLE_PYRAMID, status);
            BuildPart(legT, 2, { hX, HIP_Y - 0.45f, -0.2f }, { 0,0,0 }, { 0.08f, 0.15f, 0.08f }, cC_F, ShapeType::CYLINDER, status);
            BuildPart(legT, 2, { hX, HIP_Y - 0.95f, 0.15f }, { 0,0,0 }, { 0.22f, 0.15f, 0.45f }, cC_F, ShapeType::WEDGE, status);
            BuildPart(legT, 2, { hX, HIP_Y - 0.95f, -0.1f }, { 0,0,0 }, { 0.15f, 0.15f, 0.15f }, cC_M, ShapeType::CUBE, status);
        }
        else if (modelID == 3) {
            BuildPart(legT, 3, { hX, HIP_Y, 0 }, { 0,0,0 }, { 0.35f, 0.35f, 0.35f }, cD_F, ShapeType::SPHERE, status);
            BuildPart(legT, 3, { hX, HIP_Y - 0.4f, 0 }, { 0,0,0 }, { 0.55f, 0.85f, 0.55f }, cD_M, ShapeType::CUBE, status);
            BuildPart(legT, 3, { hX, HIP_Y - 0.4f, 0.32f }, { 0,0,0 }, { 0.35f, 0.55f, 0.15f }, cD_S, ShapeType::CUBE, status);
            BuildPart(legT, 3, { hX - dirX * 0.32f, HIP_Y - 0.4f, 0 }, { 0,0,0 }, { 0.15f, 0.55f, 0.35f }, cD_S, ShapeType::CUBE, status);
            BuildPart(legT, 3, { hX, HIP_Y - 0.85f, 0 }, { 0,0,0 }, { 0.65f, 0.3f, 0.65f }, cD_M, ShapeType::TRUNCATED_CONE, status);
            BuildPart(legT, 3, { hX, HIP_Y - 0.9f, 0.35f }, { 0,0,0 }, { 0.25f, 0.15f, 0.15f }, cD_F, ShapeType::CUBE, status);
        }
        else if (modelID == 4) {
            BuildPart(legT, 4, { hX, HIP_Y, 0 }, { 0,0,0 }, { 0.25f, 0.25f, 0.25f }, cE_F, ShapeType::SPHERE, status);
            BuildPart(legT, 4, { hX, HIP_Y - 0.4f, 0 }, { -0.1f,0,0 }, { 0.3f, 0.8f, 0.3f }, cE_M, ShapeType::CUBE, status);
            BuildPart(legT, 4, { hX, HIP_Y - 0.25f, 0.22f }, { -0.2f,0,0 }, { 0.12f, 0.35f, 0.18f }, cE_S, ShapeType::DOUBLE_PYRAMID, status);
            BuildPart(legT, 4, { hX, HIP_Y - 0.5f, 0.22f }, { -0.3f,0,0 }, { 0.1f, 0.5f, 0.25f }, cE_S, ShapeType::WEDGE, status);
            BuildPart(legT, 4, { hX, HIP_Y - 0.25f, -0.18f }, { 0.2f,0,0 }, { 0.15f, 0.25f, 0.15f }, cE_G, ShapeType::SPHERE, status);
            BuildPart(legT, 4, { hX, HIP_Y - 0.95f, 0.18f }, { 0,0,0 }, { 0.18f, 0.18f, 0.45f }, cE_F, ShapeType::DOUBLE_PYRAMID, status);
            BuildPart(legT, 4, { hX, HIP_Y - 0.95f, -0.15f }, { 0,0,0 }, { 0.15f, 0.15f, 0.25f }, cE_M, ShapeType::WEDGE, status);
        }
        };
    BuildLeg(PartType::LegLeft, cData.legID, -1.0f, legS);
    BuildLeg(PartType::LegRight, cData.legID, 1.0f, legS);

    //•Ší
    auto BuildWeapon = [&](PartType handT, EntityID parentArmFrame, int weaponID, float dirX) {
        EntityID weaponPivot = world->CreateEntity().Build();
        DirectX::XMFLOAT3 pivotOffset = { 0.0f, -0.55f, 0.0f };
        world->AddComponent<TransformComponent>(weaponPivot, TransformComponent{ .position = pivotOffset, .scale = {1.0f, 1.0f, 1.0f} });

        DirectX::XMFLOAT3 holdRot = { 0.0f, 0.0f, 0.0f };
        if (weaponID == 0 || weaponID == 1) holdRot = { 0.2f, dirX * -0.1f, 0.0f };
        else if (weaponID == 2) holdRot = { 0.4f, dirX * -0.3f, 0.0f };

        if (isEnemy) {
            world->AddComponent<EnemyPartComponent>(weaponPivot, EnemyPartComponent{ .parentID = (int)parentArmFrame, .partType = handT, .partModelID = -1, .baseOffset = pivotOffset, .baseRotation = holdRot });
        }
        else {
            PlayerPartComponent pivotPart;
            pivotPart.parentID = (int)parentArmFrame; pivotPart.partType = handT; pivotPart.partModelID = -1; pivotPart.baseOffset = pivotOffset; pivotPart.baseRotation = holdRot;
            world->AddComponent<PlayerPartComponent>(weaponPivot, pivotPart);
        }

        auto BuildWeaponPart = [&](DirectX::XMFLOAT3 localOffset, DirectX::XMFLOAT3 rot, DirectX::XMFLOAT3 scale, DirectX::XMFLOAT4 color, ShapeType shape) {
            EntityID weaponPart = world->CreateEntity().Build();
            world->AddComponent<TransformComponent>(weaponPart, TransformComponent{ .position = localOffset, .rotation = rot, .scale = scale });
            world->AddComponent<MeshComponent>(weaponPart);
            AttachMeshAndCollider(weaponPart, world, shape, color, ColliderType::Type_None, 0.0f, 0.0f, 0.0f);

            if (isEnemy) {
                world->AddComponent<EnemyPartComponent>(weaponPart, EnemyPartComponent{ .parentID = (int)weaponPivot, .partType = handT, .partModelID = weaponID, .baseOffset = localOffset, .baseRotation = rot });
            }
            else {
                world->AddComponent<PlayerPartComponent>(weaponPart, PlayerPartComponent{ .parentID = (int)weaponPivot, .partType = handT, .partModelID = weaponID, .baseOffset = localOffset, .baseRotation = rot });
            }
            };

        if (weaponID == 0) {
            DirectX::XMFLOAT4 gunColor = { 0.25f, 0.25f, 0.25f, 1.0f }; DirectX::XMFLOAT4 darkColor = { 0.15f, 0.15f, 0.15f, 1.0f }; DirectX::XMFLOAT4 barrelColor = { 0.2f, 0.2f, 0.2f, 1.0f };
            BuildWeaponPart({ 0.0f, -0.1f,  0.0f }, { 0.15f,0,0 }, { 0.04f, 0.15f, 0.06f }, darkColor, ShapeType::CUBE);
            BuildWeaponPart({ 0.0f,  0.02f, 0.05f }, { 0,0,0 }, { 0.02f, 0.12f, 0.02f }, gunColor, ShapeType::CUBE);
            BuildWeaponPart({ 0.0f,  0.1f,  0.15f }, { 0,0,0 }, { 0.06f, 0.12f, 0.35f }, gunColor, ShapeType::CUBE);
            BuildWeaponPart({ 0.0f,  0.02f, 0.2f }, { 0,0,0 }, { 0.06f, 0.08f, 0.15f }, gunColor, ShapeType::CUBE);
            BuildWeaponPart({ 0.0f, -0.12f, 0.2f }, { -0.1f,0,0 }, { 0.05f, 0.2f,  0.1f }, darkColor, ShapeType::CUBE);
            BuildWeaponPart({ 0.0f, -0.22f, 0.21f }, { -0.1f,0,0 }, { 0.06f, 0.04f, 0.11f }, gunColor, ShapeType::CUBE);
            BuildWeaponPart({ 0.0f,  0.1f,  0.45f }, { 0,0,0 }, { 0.06f, 0.1f,  0.25f }, gunColor, ShapeType::CUBE);
            BuildWeaponPart({ 0.0f,  0.1f,  0.6f }, { DirectX::XMConvertToRadians(0.0f),DirectX::XMConvertToRadians(90.0f),DirectX::XMConvertToRadians(78.5f) }, { 0.02f, 0.4f,  0.02f }, barrelColor, ShapeType::CYLINDER);
            BuildWeaponPart({ 0.0f,  0.15f,  0.75f }, { DirectX::XMConvertToRadians(0.0f),DirectX::XMConvertToRadians(90.0f),DirectX::XMConvertToRadians(78.5f) }, { 0.03f, 0.08f, 0.03f }, darkColor, ShapeType::CYLINDER);
            BuildWeaponPart({ 0.0f,  0.1f, -0.15f }, { DirectX::XMConvertToRadians(0.0f),DirectX::XMConvertToRadians(90.0f),DirectX::XMConvertToRadians(78.0f) }, { 0.03f, 0.25f, 0.03f }, barrelColor, ShapeType::CYLINDER);
            BuildWeaponPart({ 0.0f,  0.05f,-0.3f }, { 0,0,0 }, { 0.04f, 0.15f, 0.08f }, gunColor, ShapeType::CUBE);
            BuildWeaponPart({ 0.0f,  0.18f, 0.1f }, { 0,0,0 }, { 0.03f, 0.04f, 0.15f }, darkColor, ShapeType::CUBE);
            BuildWeaponPart({ 0.0f,  0.22f, 0.1f }, { DirectX::XMConvertToRadians(0.0f),DirectX::XMConvertToRadians(90.0f),DirectX::XMConvertToRadians(78.0f) }, { 0.04f, 0.2f,  0.04f }, barrelColor, ShapeType::CYLINDER);
        }
        else if (weaponID == 1) {
            DirectX::XMFLOAT4 gunColor = { 0.7f, 0.7f, 0.75f, 1.0f }; DirectX::XMFLOAT4 darkColor = { 0.3f, 0.3f, 0.35f, 1.0f }; DirectX::XMFLOAT4 energyColor = { 0.0f, 0.9f, 1.0f, 1.0f };
            BuildWeaponPart({ 0.0f, -0.1f,  0.0f }, { 0.15f,0,0 }, { 0.04f, 0.15f, 0.06f }, darkColor, ShapeType::CUBE);
            BuildWeaponPart({ 0.0f,  0.02f, 0.05f }, { -0.1f,0,0 }, { 0.03f, 0.08f, 0.03f }, gunColor, ShapeType::WEDGE);
            BuildWeaponPart({ 0.0f,  0.1f,  0.15f }, { 0,0,0 }, { 0.08f, 0.12f, 0.35f }, gunColor, ShapeType::HEXAGONAL_PRISM);
            BuildWeaponPart({ 0.0f,  0.1f, -0.08f }, { 0,0,0 }, { 0.07f, 0.12f, 0.11f }, darkColor, ShapeType::CUBE);
            BuildWeaponPart({ 0.0f, -0.05f, 0.15f }, { 0,0,0 }, { 0.06f, 0.15f, 0.08f }, darkColor, ShapeType::CYLINDER);
            BuildWeaponPart({ 0.0f, -0.15f, 0.16f }, { 0,0,0 }, { 0.04f, 0.18f, 0.06f }, energyColor, ShapeType::CYLINDER);
            BuildWeaponPart({ 0.0f,  0.1f,  0.475f }, { DirectX::XMConvertToRadians(0.0f),DirectX::XMConvertToRadians(90.0f),DirectX::XMConvertToRadians(78.5f) }, { 0.04f, 0.3f,  0.04f }, darkColor, ShapeType::CYLINDER);
            BuildWeaponPart({ 0.0f,  0.1f,  0.425f }, { 0,0,0 }, { 0.07f, 0.07f, 0.2f }, gunColor, ShapeType::HEXAGONAL_PRISM);
            BuildWeaponPart({ 0.0f,  0.1f,  0.55f }, { 0,30,30 }, { 0.07f, 0.04f, 0.07f }, gunColor, ShapeType::TORUS);
            BuildWeaponPart({ 0.0f,  0.125f,  0.65f }, { 0,30,30 }, { 0.04f, 0.02f, 0.04f }, gunColor, ShapeType::TORUS);
            BuildWeaponPart({ 0.0f,  0.18f, 0.15f }, { DirectX::XMConvertToRadians(0.0f),DirectX::XMConvertToRadians(90.0f),DirectX::XMConvertToRadians(78.5f) }, { 0.02f, 0.3f,  0.02f }, energyColor, ShapeType::CYLINDER);
            BuildWeaponPart({ 0.0f,  0.22f, 0.0f }, { 0,0,0 }, { 0.04f, 0.06f, 0.15f }, darkColor, ShapeType::WEDGE);
        }
        else if (weaponID == 2) {
            DirectX::XMFLOAT4 gripColor = { 0.4f, 0.4f, 0.4f, 1.0f }; DirectX::XMFLOAT4 beamColor = { 1.0f, 0.1f, 0.5f, 0.8f };
            BuildWeaponPart({ 0.0f,  0.1f, 0.1f }, { DirectX::XMConvertToRadians(0.0f),DirectX::XMConvertToRadians(90.0f),DirectX::XMConvertToRadians(40.0f) }, { 0.03f, 0.2f, 0.03f }, gripColor, ShapeType::CYLINDER);
            BuildWeaponPart({ 0.0f,  0.1f, 0.1f }, { DirectX::XMConvertToRadians(0.0f),DirectX::XMConvertToRadians(90.0f),DirectX::XMConvertToRadians(40.0f) }, { 0.04f, 0.06f, 0.04f }, gripColor, ShapeType::TORUS);
            BuildWeaponPart({ 0.0f,  0.55f, 0.5f }, { DirectX::XMConvertToRadians(0.0f),DirectX::XMConvertToRadians(90.0f),DirectX::XMConvertToRadians(40.0f) }, { 0.04f, 1.1f, 0.04f }, beamColor, ShapeType::CYLINDER);
        }
        };

    BuildWeapon(PartType::HandLeft, frameArmL, cData.weaponLeftID, -1.0f);
    BuildWeapon(PartType::HandRight, frameArmR, cData.weaponRightID, 1.0f);

    return playerCore;
}
}