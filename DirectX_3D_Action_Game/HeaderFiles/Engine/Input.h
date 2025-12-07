#pragma once
#include <Windows.h>
#include <array>

class Input {
public:
    void Initialize();
    void Update();

    // 特定のキーが押されているか (例: 'W', VK_UP)
    bool IsKey(int key) const;
    // 特定のキーが押された瞬間か
    bool IsKeyDown(int key) const;
    // 特定のキーが離された瞬間か
    bool IsKeyUp(int key) const;

private:
    std::array<bool, 256> currentKeys;
    std::array<bool, 256> previousKeys;
};
