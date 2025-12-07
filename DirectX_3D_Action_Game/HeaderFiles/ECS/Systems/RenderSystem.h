/*===================================================================
//ファイル:RenderSystem.h
//概要:MeshComponentを持つエンティティを描画するシステム
=====================================================================*/
#pragma once
#include "ECS/System.h"
#include "ECS/World.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/CameraComponent.h"
#include "App/Game.h"
#include <d3d11.h>
#include <DirectXMath.h>

using namespace DirectX;

struct ConstantBufferData {
    XMMATRIX transform;
};

class RenderSystem : public System {
public:
    void Init(World* world) override {
        System::Init(world);

        // シェーダーの読み込み
        Graphics* graphics = Game::GetInstance()->GetGraphics();
        
        if (!graphics->CreateVertexShader(L"Shaders/SimpleVS.hlsl", &pVS, &pInputLayout)) {
            MessageBoxW(NULL, L"Vertex Shader Load Failed", L"Error", MB_OK);
        }
        if (!graphics->CreatePixelShader(L"Shaders/SimplePS.hlsl", &pPS)) {
            MessageBoxW(NULL, L"Pixel Shader Load Failed", L"Error", MB_OK);
        }

        graphics->CreateConstantBuffer(sizeof(ConstantBufferData), &pConstantBuffer);
    }

    void Draw() override {
        Graphics* graphics = Game::GetInstance()->GetGraphics();
        ID3D11DeviceContext* context = graphics->GetContext();

        //アクティブなカメラを探す
        XMMATRIX viewProj = XMMatrixIdentity();
        auto registry = pWorld->GetRegistry();

        for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
            if (registry->HasComponent<CameraComponent>(id)) {
                auto& cam = registry->GetComponent<CameraComponent>(id);
                // ビュー行列 * プロジェクション行列
                viewProj = cam.view * cam.projection;
                break; // カメラは1つと仮定してbreak
            }
        }

        // 1. パイプライン設定（全体共通）
        context->IASetInputLayout(pInputLayout.Get());
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        context->VSSetShader(pVS.Get(), nullptr, 0);
        context->PSSetShader(pPS.Get(), nullptr, 0);

        context->VSSetConstantBuffers(0, 1, pConstantBuffer.GetAddressOf());

        static float angle = 0.0f;
        angle += 0.02f; // 回転スピード

        // ※ECSの正規ループ（本来はViewなどを使うが、簡易的に全ID走査）
        for (EntityID id = 0; id < ECSConfig::MAX_ENTITIES; ++id) {
            // TransformとMeshを持っているか確認
            if (!registry->HasComponent<TransformComponent>(id)) continue;
            if (!registry->HasComponent<MeshComponent>(id)) continue;

            auto& mesh = registry->GetComponent<MeshComponent>(id);
            auto& trans = registry->GetComponent<TransformComponent>(id);
            // ★修正: TransformComponentの値をすべて反映させる計算
            // -------------------------------------------------------
            // 1. 拡大縮小行列 (Scale)
            XMMATRIX S = XMMatrixScaling(trans.scale.x, trans.scale.y, trans.scale.z);

            // 2. 回転行列 (Rotation: ラジアン)
            XMMATRIX R = XMMatrixRotationRollPitchYaw(trans.rotation.x, trans.rotation.y + angle, trans.rotation.z);

            // 3. 平行移動行列 (Translation)
            XMMATRIX T = XMMatrixTranslation(trans.position.x, trans.position.y, trans.position.z);

            // 4. ワールド行列の合成 (順番重要: S -> R -> T)
            // 拡大してから、回して、移動する
            XMMATRIX worldMatrix = S * R * T;

            // ※テスト用: プレイヤーだけクルクル回してみる（IDが0なら回す、のような簡易判定）
            if (id == 0) {
                static float rotY = 0.0f; rotY += 0.02f;
                worldMatrix = S * XMMatrixRotationY(rotY) * T;
            }

            //// 5. 転置してGPUへ送る
            //ConstantBufferData cbData;
            //cbData.transform = XMMatrixTranspose(worldMatrix);

            //context->UpdateSubresource(pConstantBuffer.Get(), 0, nullptr, &cbData, 0, 0);
            //// -------------------------------------------------------

            //// 頂点バッファをセット
            //ID3D11Buffer* vbs[] = { mesh.pVertexBuffer.Get() };
            //context->IASetVertexBuffers(0, 1, vbs, &mesh.stride, &mesh.offset);

            //// 描画実行！
            //context->Draw(mesh.vertexCount, 0);
            XMMATRIX wvp = worldMatrix * viewProj;

            ConstantBufferData cbData;
            cbData.transform = XMMatrixTranspose(wvp); // 転置して送信
            context->UpdateSubresource(pConstantBuffer.Get(), 0, nullptr, &cbData, 0, 0);

            // 描画
            ID3D11Buffer* vbs[] = { mesh.pVertexBuffer.Get() };
            context->IASetVertexBuffers(0, 1, vbs, &mesh.stride, &mesh.offset);
            context->IASetIndexBuffer(mesh.pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
            context->DrawIndexed(mesh.indexCount, 0, 0);
        }
    }

private:
    ComPtr<ID3D11VertexShader> pVS;
    ComPtr<ID3D11PixelShader> pPS;
    ComPtr<ID3D11InputLayout> pInputLayout;
    ComPtr<ID3D11Buffer> pConstantBuffer;
};
