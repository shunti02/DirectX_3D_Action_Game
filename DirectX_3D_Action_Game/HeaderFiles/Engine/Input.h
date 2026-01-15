#pragma once
#include <Windows.h>
#include <array>
#include <DirectXMath.h>
class Input {
public:
    void Initialize(HWND hWnd);
    void Update();

    //キーボード
    // 特定のキーが押されているか
    bool IsKey(int key) const;
    // 特定のキーが押された瞬間か
    bool IsKeyDown(int key) const;
    // 特定のキーが離された瞬間か
    bool IsKeyUp(int key) const;

    //マウス
    bool IsMouseKey(int button) const;
    bool IsMouseKeyDown(int button) const;
    bool IsMouseKeyUp(int Key) const;

    // ---追加: マウスカーソル座標 ---
    DirectX::XMFLOAT2 GetMousePosition() const;

private:
    HWND m_hWnd = nullptr; // 座標変換用に保持
    std::array<bool, 256> currentKeys;
    std::array<bool, 256> previousKeys;
};
