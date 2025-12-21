#pragma once
#include <DirectXMath.h>

enum class PartType {
    Head,
	EarLeft,
	EarRight,
    Body,
	ShoulderLeft,
	ShoulderRight,
    ArmLeft,
    ArmRight,
    HandLeft,
    HandRight,
	LegLeft,
    LegRight
};

struct PlayerPartComponent {
    int parentID = -1;      // 本体のEntityID
    PartType partType;      // 部位の種類

    // 基準となるオフセット位置（本体からどれくらい離れているか）
    DirectX::XMFLOAT3 baseOffset = { 0.0f, 0.0f, 0.0f };

    // 基準となる回転（初期ポーズ）
    DirectX::XMFLOAT3 baseRotation = { 0.0f, 0.0f, 0.0f };
};
