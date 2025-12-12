#include "Engine/Input.h"

void Input::Initialize() {
    currentKeys.fill(false);
    previousKeys.fill(false);
}

void Input::Update() {
    previousKeys = currentKeys;
    for (int i = 0; i < 256; ++i) {
        // ÅãˆÊƒrƒbƒg‚ª—§‚Á‚Ä‚¢‚ê‚Î‰Ÿ‚³‚ê‚Ä‚¢‚é
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