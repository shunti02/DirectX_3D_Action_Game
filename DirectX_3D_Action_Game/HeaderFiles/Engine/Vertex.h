#pragma once
#include <DirectXMath.h>

struct Vertex {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT4 color; 

    Vertex() = default;

    Vertex(float x, float y, float z, float r, float g, float b, float a)
        : position(x, y, z), normal(0.0f, 1.0f, 0.0f), color(r, g, b, a) {
    }
    Vertex(float x, float y, float z, float nx, float ny, float nz, float r, float g, float b, float a)
        : position(x, y, z), normal(nx, ny, nz), color(r, g, b, a) {
    }
};