/*===================================================================
//ファイル:RenderSystem.cpp
//概要:MeshComponentを持つエンティティを描画するシステム
//修正: 定数バッファのセット漏れを修正し、描画ステートを完全にリセットする
=====================================================================*/
#include "ECS/Systems/RenderSystem.h"
#include "App/Main.h"


void RenderSystem::Init(World* world) {
	System::Init(world);


	Graphics* graphics = Game::GetInstance()->GetGraphics();
	ID3D11Device* device = graphics->GetDevice();


	// シェーダー読み込み
	if (!graphics->CreateVertexShader(L"Shaders/SimpleVS.hlsl", &pVS, &pInputLayout)) {
		MessageBoxW(NULL, L"Vertex Shader Load Failed", L"Error", MB_OK);
	}
	if (!graphics->CreatePixelShader(L"Shaders/SimplePS.hlsl", &pPS)) {
		MessageBoxW(NULL, L"Pixel Shader Load Failed", L"Error", MB_OK);
	}


	graphics->CreateConstantBuffer(sizeof(ConstantBufferData), &pConstantBuffer);


	HRESULT hr;

	// -----------------------------------------------------
	// 1. ラスタライザーステート作成
	// -----------------------------------------------------
	// [A] 通常描画 (Solid)
	D3D11_RASTERIZER_DESC solidDesc = {};
	solidDesc.FillMode = D3D11_FILL_SOLID;
	solidDesc.CullMode = D3D11_CULL_BACK;
	solidDesc.FrontCounterClockwise = TRUE;
	solidDesc.DepthClipEnable = TRUE;
	hr = device->CreateRasterizerState(&solidDesc, &pSolidState);
	if (FAILED(hr)) MessageBoxW(NULL, L"Failed: SolidState", L"Error", MB_OK);

	// [B] ワイヤーフレーム (Debug)
	D3D11_RASTERIZER_DESC wfDesc = {};
	wfDesc.FillMode = D3D11_FILL_WIREFRAME;
	wfDesc.CullMode = D3D11_CULL_NONE;
	wfDesc.FrontCounterClockwise = TRUE;
	wfDesc.DepthClipEnable = TRUE;
	wfDesc.DepthBias = -1000;
	wfDesc.SlopeScaledDepthBias = -1.0f;
	hr = device->CreateRasterizerState(&wfDesc, &pWireframeState);
	if (FAILED(hr)) MessageBoxW(NULL, L"Failed: WireframeState", L"Error", MB_OK);

	// -----------------------------------------------------
	// 2. 深度ステンシルステート作成
	// -----------------------------------------------------
	D3D11_DEPTH_STENCIL_DESC dsDesc = {};
	dsDesc.DepthEnable = TRUE;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
	dsDesc.StencilEnable = FALSE;
	hr = device->CreateDepthStencilState(&dsDesc, &pDepthState);
	if (FAILED(hr)) MessageBoxW(NULL, L"Failed: DepthState", L"Error", MB_OK);

	// -----------------------------------------------------
	// 3. ブレンドステート作成
	// -----------------------------------------------------
	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	hr = device->CreateBlendState(&blendDesc, &pBlendState);
	if (FAILED(hr)) MessageBoxW(NULL, L"Failed: BlendState", L"Error", MB_OK);

	// デバッグ用メッシュの作成
	MeshData boxData = GeometryGenerator::CreateMesh(ShapeType::CUBE, Colors::Red);
	CreateDebugMesh(boxData, debugMeshBox);


	MeshData capData = GeometryGenerator::CreateMesh(ShapeType::CAPSULE, Colors::Red);
	CreateDebugMesh(capData, debugMeshCapsule);

	// ★追加: SPHERE (球)
	MeshData sphereData = GeometryGenerator::CreateMesh(ShapeType::SPHERE, Colors::Red);
	CreateDebugMesh(sphereData, debugMeshSphere);
}

void RenderSystem::Update(float dt) {
	// Zキーが押された瞬間、表示フラグを反転させる
	if (Game::GetInstance()->GetInput()->IsKeyDown('Z')) {
		showColliders = !showColliders;
	}
}

void RenderSystem::Draw() {
	Graphics* graphics = Game::GetInstance()->GetGraphics();
	ID3D11DeviceContext* context = graphics->GetContext();


	// カメラ行列計算
	XMMATRIX viewProj = XMMatrixIdentity();
	auto registry = pWorld->GetRegistry();
	for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
		if (registry->HasComponent<CameraComponent>(id)) {
			auto& cam = registry->GetComponent<CameraComponent>(id);
			viewProj = cam.view * cam.projection;
			break;
		}
	}

	// =====================================================
// ★重要: ステートの強制リセット
// =====================================================
	context->IASetInputLayout(pInputLayout.Get());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->VSSetShader(pVS.Get(), nullptr, 0);
	context->PSSetShader(pPS.Get(), nullptr, 0);

	context->VSSetConstantBuffers(0, 1, pConstantBuffer.GetAddressOf());


	if (pDepthState) {
		context->OMSetDepthStencilState(pDepthState.Get(), 1);
	}


	if (pBlendState) {
		float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		context->OMSetBlendState(pBlendState.Get(), blendFactor, 0xffffffff);
	}

	if (pSolidState) {
		context->RSSetState(pSolidState.Get());
	}

	D3D11_VIEWPORT vp;
	vp.Width = (float)Config::SCREEN_WIDTH;
	vp.Height = (float)Config::SCREEN_HEIGHT;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	context->RSSetViewports(1, &vp);

	// =====================================================
// 通常描画ループ
// =====================================================
	for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
		if (!registry->HasComponent<TransformComponent>(id)) continue;
		if (!registry->HasComponent<MeshComponent>(id)) continue;


		auto& mesh = registry->GetComponent<MeshComponent>(id);
		auto& trans = registry->GetComponent<TransformComponent>(id);


		XMMATRIX world = CalculateWorldMatrix(trans);
		XMMATRIX wvp = world * viewProj;


		UpdateConstantBuffer(context, wvp);


		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		context->IASetVertexBuffers(0, 1, mesh.pVertexBuffer.GetAddressOf(), &stride, &offset);
		context->IASetIndexBuffer(mesh.pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		context->DrawIndexed(mesh.indexCount, 0, 0);
	}

	// =====================================================
// デバッグ描画
// =====================================================
	if (showColliders && pWireframeState) {
		context->RSSetState(pWireframeState.Get());


		for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
			if (!registry->HasComponent<TransformComponent>(id)) continue;
			if (!registry->HasComponent<ColliderComponent>(id)) continue;


			auto& col = registry->GetComponent<ColliderComponent>(id);
			auto& trans = registry->GetComponent<TransformComponent>(id);


			if (col.type == ColliderType::Type_None) continue;

			XMMATRIX world = XMMatrixIdentity();
			MeshComponent* pDebugMesh = nullptr;

			// ★修正: コライダータイプごとのサイズ計算とメッシュ選択
			if (col.type == ColliderType::Type_Box) {
				world *= XMMatrixScaling(
					col.size.x * trans.scale.x,
					col.size.y * trans.scale.y,
					col.size.z * trans.scale.z
				);
				pDebugMesh = &debugMeshBox;
			}
			else if (col.type == ColliderType::Type_Sphere) {
				// 1. 本来あるべき半径を計算
				float trueRadius = col.radius * trans.scale.x;

				// 2. メッシュ(半径0.5)に合わせて倍率を補正
				float scaleFactor = trueRadius * 2.0f;

				// 3. 適用
				world *= XMMatrixScaling(scaleFactor, scaleFactor, scaleFactor);
				pDebugMesh = &debugMeshSphere;
			}
			else if (col.type == ColliderType::Type_Capsule) {
				// カプセルの場合
				float sR = (col.radius * trans.scale.x) / 0.5f;
				float sH = (col.height * trans.scale.y) / 2.0f;
				world *= XMMatrixScaling(sR, sH, sR);
				pDebugMesh = &debugMeshCapsule;
			}

			if (pDebugMesh != nullptr) {
				XMMATRIX R = XMMatrixRotationRollPitchYaw(trans.rotation.x, trans.rotation.y, trans.rotation.z);
				XMMATRIX T = XMMatrixTranslation(trans.position.x, trans.position.y, trans.position.z);
				world *= R * T;

				XMMATRIX wvp = world * viewProj;
				UpdateConstantBuffer(context, wvp);

				UINT stride = sizeof(Vertex);
				UINT offset = 0;
				context->IASetVertexBuffers(0, 1, pDebugMesh->pVertexBuffer.GetAddressOf(), &stride, &offset);
				context->IASetIndexBuffer(pDebugMesh->pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
				context->DrawIndexed(pDebugMesh->indexCount, 0, 0);
			}
		}

		if (pSolidState) context->RSSetState(pSolidState.Get());
	}
}

XMMATRIX RenderSystem::CalculateWorldMatrix(const TransformComponent& t) {
	XMMATRIX S = XMMatrixScaling(t.scale.x, t.scale.y, t.scale.z);
	XMMATRIX R = XMMatrixRotationRollPitchYaw(t.rotation.x, t.rotation.y, t.rotation.z);
	XMMATRIX T = XMMatrixTranslation(t.position.x, t.position.y, t.position.z);
	return S * R * T;
}

void RenderSystem::UpdateConstantBuffer(ID3D11DeviceContext* context, XMMATRIX wvp) {
	ConstantBufferData cbData;
	cbData.transform = XMMatrixTranspose(wvp);
	context->UpdateSubresource(pConstantBuffer.Get(), 0, nullptr, &cbData, 0, 0);
}

void RenderSystem::CreateDebugMesh(const MeshData& data, MeshComponent& outMesh) {
	outMesh.vertexCount = (UINT)data.vertices.size();
	outMesh.indexCount = (UINT)data.indices.size();
	outMesh.stride = sizeof(Vertex);
	Graphics* g = Game::GetInstance()->GetGraphics();
	g->CreateVertexBuffer(data.vertices, outMesh.pVertexBuffer.GetAddressOf());
	g->CreateIndexBuffer(data.indices, outMesh.pIndexBuffer.GetAddressOf());
}