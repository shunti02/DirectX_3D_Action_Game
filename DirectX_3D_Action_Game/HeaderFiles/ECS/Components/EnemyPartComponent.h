/*===================================================================
// ファイル: EnemyPartComponent.h
// 概要: エネミーの構成パーツ情報
=====================================================================*/
#pragma once
#include <DirectXMath.h>

// エネミーのパーツ識別用
enum class EnemyPartType {
    Body,       // 胴体
    Head,       // 頭
    ArmLeft,    // 左腕
    ArmRight,   // 右腕
    LegLeft,    // 左足（追加）
    LegRight,   // 右足（追加）
    Weapon,     // 武器
    Core,       // コア
    Wing,       // 羽
    Thruster,   // スラスター（追加：常に揺らす）
    Shield,     // シールド（追加）
    Ring        // リング（追加：回転させる）
};

struct EnemyPartComponent {
    int parentID = -1;
    EnemyPartType partType;
    DirectX::XMFLOAT3 baseOffset = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 baseRotation = { 0.0f, 0.0f, 0.0f };
};
