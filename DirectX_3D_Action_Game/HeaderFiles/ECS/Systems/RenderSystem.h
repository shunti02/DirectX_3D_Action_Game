/*===================================================================
//ファイル:RenderSystem.h
//概要:MeshComponentを持つエンティティを描画するシステム
//修正: 定数バッファのセット漏れを修正し、描画ステートを完全にリセットする
=====================================================================*/
#pragma once
#include "ECS/System.h"
#include "ECS/World.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/CameraComponent.h"
#include "ECS/Components/ColliderComponent.h"
#include "App/Game.h"
#include "Engine/GeometryGenerator.h"
#include "Engine/Colors.h"
#include <d3d11.h>
#include <DirectXMath.h>


using namespace DirectX;


struct ConstantBufferData {
	XMMATRIX transform;
};


class RenderSystem : public System {
public:
	// デバッグ表示のON/OFF切り替えフラグ
	bool showColliders = false;


	void Init(World* world) override;
	void Update(float dt) override;
	void Draw() override;


private:
	ComPtr<ID3D11VertexShader> pVS;
	ComPtr<ID3D11PixelShader> pPS;
	ComPtr<ID3D11InputLayout> pInputLayout;
	ComPtr<ID3D11Buffer> pConstantBuffer;


	ComPtr<ID3D11RasterizerState> pWireframeState;
	ComPtr<ID3D11RasterizerState> pSolidState;
	ComPtr<ID3D11DepthStencilState> pDepthState;
	ComPtr<ID3D11BlendState> pBlendState;


	MeshComponent debugMeshBox;
	MeshComponent debugMeshCapsule;
	MeshComponent debugMeshSphere;


	XMMATRIX CalculateWorldMatrix(const TransformComponent& t);
	void UpdateConstantBuffer(ID3D11DeviceContext* context, XMMATRIX wvp);
	void CreateDebugMesh(const MeshData& data, MeshComponent& outMesh);
};

