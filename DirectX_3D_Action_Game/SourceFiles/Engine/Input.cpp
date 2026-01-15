#include "Engine/Input.h"

void Input::Initialize(HWND hWnd) {
    m_hWnd = hWnd;
    currentKeys.fill(false);
    previousKeys.fill(false);
}

void Input::Update() {
    previousKeys = currentKeys;
    for (int i = 0; i < 256; ++i) {
        // 最上位ビットが立っていれば押されている
        currentKeys[i] = (GetAsyncKeyState(i) & 0x8000) != 0;
    }
}
// --- キーボード実装 ---
bool Input::IsKey(int key) const {
    return currentKeys[key];
}

bool Input::IsKeyDown(int key) const {
    return currentKeys[key] && !previousKeys[key];
}

bool Input::IsKeyUp(int key) const {
    return !currentKeys[key] && previousKeys[key];
}
//マウス入力
int GetMouseVK(int button) {
    switch (button) {
    case 0: return VK_LBUTTON;//左クリック
    case 1: return VK_RBUTTON;//右クリック
    case 2: return VK_MBUTTON;//中クリック
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

// ---追加: マウス座標取得の実装 ---
DirectX::XMFLOAT2 Input::GetMousePosition() const {
    POINT pos;
    GetCursorPos(&pos); // スクリーン座標を取得
    ScreenToClient(m_hWnd, &pos); // ウィンドウ内座標に変換
    return DirectX::XMFLOAT2((float)pos.x, (float)pos.y);
}