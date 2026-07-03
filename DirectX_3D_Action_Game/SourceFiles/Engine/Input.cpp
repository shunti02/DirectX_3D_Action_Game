#include "Engine/Input.h"

void Input::Initialize(HWND hWnd) {
    m_hWnd = hWnd;
    currentKeys.fill(false);
    previousKeys.fill(false);
}

void Input::Update() {
    m_mouseWheelDelta = 0.0f;
    previousKeys = currentKeys;
    for (int i = 0; i < 256; ++i) {
        currentKeys[i] = (GetAsyncKeyState(i) & 0x8000) != 0;
    }
}
bool Input::IsKey(int key) const {
    return currentKeys[key];
}

bool Input::IsKeyDown(int key) const {
    return currentKeys[key] && !previousKeys[key];
}

bool Input::IsKeyUp(int key) const {
    return !currentKeys[key] && previousKeys[key];
}
//ƒ}ƒEƒX“ü—Í
int GetMouseVK(int button) {
    switch (button) {
    case 0: return VK_LBUTTON;
    case 1: return VK_RBUTTON;
    case 2: return VK_MBUTTON;
    default: return 0;
    }
}

bool Input::IsMouseKey(int button) const {
    int key = GetMouseVK(button);
    return IsKey(key);
}

bool Input::IsMouseKeyDown(int button) const {
    int key = GetMouseVK(button);
    return IsKeyDown(key);
}

bool Input::IsMouseKeyUp(int button) const {
    int key = GetMouseVK(button);
    return IsKeyUp(key);
}

DirectX::XMFLOAT2 Input::GetMousePosition() const {
    POINT pos;
    GetCursorPos(&pos);
    ScreenToClient(m_hWnd, &pos);
    return DirectX::XMFLOAT2((float)pos.x, (float)pos.y);
}