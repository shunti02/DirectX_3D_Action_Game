/*===================================================================
//ファイル:MeshComponent.h
//概要:描画に必要な頂点バッファ情報を持つコンポーネント
=====================================================================*/
#pragma once
#include <d3d11.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

struct MeshComponent {
    ComPtr<ID3D11Buffer> pVertexBuffer; // 頂点データの本体
    ComPtr<ID3D11Buffer> pIndexBuffer;
    UINT vertexCount;                   // 頂点の数（三角形なら3）
    UINT indexCount;
    UINT stride;                        // 1頂点のデータサイズ
    UINT offset;                        // オフセット（通常0）

    MeshComponent() : vertexCount(0), indexCount(0), stride(0), offset(0) {}
};
