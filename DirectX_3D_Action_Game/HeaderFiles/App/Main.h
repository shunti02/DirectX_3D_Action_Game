#pragma once
#include <Windows.h>
#include <vector>
#include <string> 
#include "Engine/Graphics.h"

#define ASSET(path)"Asset/"path

#define METER(val) (val * 1.0f)
#define CMETER(val) (val * 0.01f)
namespace Config {
	constexpr int SCREEN_WIDTH = 1280;
	constexpr int SCREEN_HEIGHT = 720;
	constexpr const char* WINDOW_TITLE = "DirectX 3D Action Game";
	constexpr int TARGET_FPS = 60;

}
namespace AppLog {
	extern std::vector<std::string> logs;
	void AddLog(const char* fmt, ...);
	void Clear();
}
#define DebugLog(...) AppLog::AddLog(__VA_ARGS__)

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void Init(HWND hWnd);
void UnInit();
void Update(float deltaTime);
void Draw();