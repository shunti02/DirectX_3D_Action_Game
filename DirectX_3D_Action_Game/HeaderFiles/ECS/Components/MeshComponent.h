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

    UINT vertexCount = 0;   // 頂点の数
    UINT indexCount = 0;
    UINT stride = 0;        // 1頂点のデータサイズ
    UINT offset = 0;        // オフセット
};
