/*===================================================================
//ファイル:TransformComponent.h
//制作日:2025/12/06
//概要:基本コンポーネント
//制作者:伊藤駿汰
//-------------------------------------------------------------------
//更新履歴:
//2025/12/06:新規作成
=====================================================================*/
#ifndef TRANSFORMCOMPONENT_H
#define TRANSFORMCOMPONENT_H
#include <DirectXMath.h>

struct TransformComponent {
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 rotation;
	DirectX::XMFLOAT3 scale;
	TransformComponent(
		float x = 0.0f, float y = 0.0f, float z = 0,
		float rx = 0.0f, float ry = 0.0f, float rz = 0.0f,
		float sx = 1.0f, float sy = 1.0f, float sz = 1.0f)
	{
		position = { x,y,z };
		rotation = { rx,ry,rz };
		scale = { sx,sy,sz };
	}
};
#endif //TRANSFORMCOMPONENT_H
