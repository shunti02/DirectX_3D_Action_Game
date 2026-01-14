/*===================================================================
// ファイル: UISystem.h
// 概要: ImGuiを使用してゲーム内ステータスを可視化するシステム
=====================================================================*/
#pragma once
#include "ECS/System.h"

// 前方宣言
class Graphics;

class UISystem : public System {
public:
	void Update(float dt) override;
	//Direct2Dによる描画
	void Draw(Graphics* pGraphics);
};