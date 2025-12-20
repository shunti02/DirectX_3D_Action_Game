/*===================================================================
// ファイル: UISystem.h
// 概要: ImGuiを使用してゲーム内ステータスを可視化するシステム
=====================================================================*/
#pragma once
#include "ECS/System.h"

class UISystem : public System {
public:
	void Update(float dt) override;
};