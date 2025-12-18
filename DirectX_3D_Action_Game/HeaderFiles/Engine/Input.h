#pragma once
#include <Windows.h>
#include <array>

class Input {
public:
    void Initialize();
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
private:
    std::array<bool, 256> currentKeys;
    std::array<bool, 256> previousKeys;
};
