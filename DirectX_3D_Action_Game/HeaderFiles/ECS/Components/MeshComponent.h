#pragma once
#include <d3d11.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

struct MeshComponent {
    ComPtr<ID3D11Buffer> pVertexBuffer;
    ComPtr<ID3D11Buffer> pIndexBuffer;

    UINT vertexCount = 0;
    UINT indexCount = 0;
    UINT stride = 0;
    UINT offset = 0;
};
