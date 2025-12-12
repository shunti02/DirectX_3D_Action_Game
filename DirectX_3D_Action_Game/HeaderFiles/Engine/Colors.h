/*===================================================================
//ファイル:Colors.h
//概要:よく使う色の定義
=====================================================================*/
#pragma once
#include <DirectXMath.h>

namespace Colors {
    //                                         R     G     B     A
    static const DirectX::XMFLOAT4 White = { 1.0f, 1.0f, 1.0f, 1.0f };
    static const DirectX::XMFLOAT4 Black = { 0.0f, 0.0f, 0.0f, 1.0f };
    static const DirectX::XMFLOAT4 Red = { 1.0f, 0.0f, 0.0f, 1.0f };
    static const DirectX::XMFLOAT4 Green = { 0.0f, 1.0f, 0.0f, 1.0f };
    static const DirectX::XMFLOAT4 Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
    static const DirectX::XMFLOAT4 Yellow = { 1.0f, 1.0f, 0.0f, 1.0f };
    static const DirectX::XMFLOAT4 Cyan = { 0.0f, 1.0f, 1.0f, 1.0f }; // 水色
    static const DirectX::XMFLOAT4 Magenta = { 1.0f, 0.0f, 1.0f, 1.0f }; // 紫
    static const DirectX::XMFLOAT4 Gray = { 0.5f, 0.5f, 0.5f, 1.0f };

    // 好きな色を追加できます
    static const DirectX::XMFLOAT4 Orange = { 1.0f, 0.5f, 0.0f, 1.0f };
}