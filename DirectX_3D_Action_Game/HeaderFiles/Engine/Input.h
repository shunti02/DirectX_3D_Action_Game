#pragma once
#include <Windows.h>
#include <array>
#include <DirectXMath.h>
class Input {
public:
    void Initialize(HWND hWnd);
    void Update();

    bool IsKey(int key) const;
    bool IsKeyDown(int key) const;
    bool IsKeyUp(int key) const;

    bool IsMouseKey(int button) const;
    bool IsMouseKeyDown(int button) const;
    bool IsMouseKeyUp(int Key) const;
    DirectX::XMFLOAT2 GetMousePosition() const;
    float GetMouseWheel() const { return m_mouseWheelDelta; }
    void AddMouseWheelDelta(float delta) { m_mouseWheelDelta += delta; }
    void ResetMouseWheel() { m_mouseWheelDelta = 0.0f; }

private:
    HWND m_hWnd = nullptr;
    std::array<bool, 256> currentKeys;
    std::array<bool, 256> previousKeys;

    float m_mouseWheelDelta = 0.0f;
};
