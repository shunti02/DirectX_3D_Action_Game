#pragma once
#include <DirectXMath.h>

enum class PartType {
    Head,
	EarLeft,
	EarRight,
    Body,
	Waist,
	ShoulderLeft,
	ShoulderRight,
    ArmLeft,
    ArmRight,
    HandLeft,
    HandRight,
	LegLeft,
    LegRight
};

struct PartStatus {
    float hp = 0.0f;
    float attack = 0.0f;
    float defense = 0.0f;
    float speed = 0.0f;
    float weight = 0.0f;
};

struct PlayerPartComponent {
    int parentID = -1;
    PartType partType;
    int partModelID = 0;
    DirectX::XMFLOAT3 baseOffset = { 0.0f, 0.0f, 0.0f };

    DirectX::XMFLOAT3 baseRotation = { 0.0f, 0.0f, 0.0f };

    PartStatus status;
};
