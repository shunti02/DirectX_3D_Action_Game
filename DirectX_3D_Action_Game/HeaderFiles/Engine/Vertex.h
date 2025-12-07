/*===================================================================
//ファイル:Vertex.h
//概要:頂点レイアウトの定義
=====================================================================*/
#pragma once
#include <DirectXMath.h>

struct Vertex {
    DirectX::XMFLOAT3 position; // 位置 (x, y, z)
    DirectX::XMFLOAT4 color;    // 色 (r, g, b, a)

    Vertex() = default;
    Vertex(float x, float y, float z, float r, float g, float b, float a)
        : position(x, y, z), color(r, g, b, a) {}
};
