#include "App/Main.h"
#include "App/Game.h"
#include <iostream>
#include <cstdio>
#include <cstdarg>
#include <crtdbg.h>
#include "../ImGui/imgui.h"
#pragma comment(lib, "winmm.lib")

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//ログ
namespace AppLog {
    std::vector<std::string> logs;

    void AddLog(const char* fmt, ...) {
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
        buf[IM_ARRAYSIZE(buf) - 1] = 0;
        va_end(args);

        OutputDebugString(buf);
        OutputDebugString("\n");

        logs.push_back(std::string(buf));

        if (logs.size() > 1000) {
            logs.erase(logs.begin());
        }
    }

    void Clear() {
        logs.clear();
    }
}
/*----------------------------------------------------------
//内部変数・定数
------------------------------------------------------------*/
namespace {
    std::unique_ptr<Game> g_Game;
    //FPS管理用
    LARGE_INTEGER g_TimeFreq;
    LARGE_INTEGER g_TimeStart;
    LARGE_INTEGER g_TimeEnd;
    float g_FrameTime = 0.0f;
}

//プロトタイプ宣言
void InitFPS();
bool IsFrameReady();
//エントリーポイント
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	//メモリーリークチェック
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    timeBeginPeriod(1);

    //ウィンドウクラス登録
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "DX11GameClass";
    if (!RegisterClassEx(&wc)) return -1;

    //ウィンドウ生成
    RECT rc = { 0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hWnd = CreateWindow(
        "DX11GameClass",Config::WINDOW_TITLE,WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,rc.right - rc.left, rc.bottom - rc.top,
        NULL, NULL, hInstance, NULL
    );

    if (!hWnd) return -1;

    //ウィンドウ表示
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    //Gameクラスの生成と初期化
    Init(hWnd);

    InitFPS();
    
    MSG msg = { 0 };
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            if (!IsFrameReady())continue;
            //Gameクラスの更新・描画
            Update(g_FrameTime);
            Draw();
        }
    }
    //終了処理
    UnInit();
    timeEndPeriod(1);

    return (int)msg.wParam;
}

//ウィンドウプロシージャ
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) PostQuitMessage(0);
        return 0;

        //マウスホイール処理
    case WM_MOUSEWHEEL:
        if (auto game = Game::GetInstance()) {
            if (auto input = game->GetInput()) {
                float delta = (float)GET_WHEEL_DELTA_WPARAM(wParam);
                input->AddMouseWheelDelta(delta);
            }
        }
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void Init(HWND hWnd)
{
    g_Game = std::make_unique<Game>();
    if (!g_Game->Initialize(hWnd)) {
        MessageBox(NULL, "Initialization Failed", "Error", MB_OK);
        exit(-1);
    }
}

void UnInit()
{
    if (g_Game) {
        g_Game->Shutdown();
        g_Game.reset();
    }
}

void Update(float deltaTime)
{
    if (g_Game) g_Game->Update(deltaTime);
}

void Draw()
{
    if (g_Game) g_Game->Draw();
}

//FPS制御
void InitFPS()
{
    QueryPerformanceFrequency(&g_TimeFreq);
    QueryPerformanceCounter(&g_TimeStart);
}

bool IsFrameReady()
{
    
    QueryPerformanceCounter(&g_TimeEnd);
    double elapsed = static_cast<double>(g_TimeEnd.QuadPart - g_TimeStart.QuadPart) / static_cast<double>(g_TimeFreq.QuadPart);
    double targetTime = 1.0 / Config::TARGET_FPS;
    if (elapsed < targetTime)return false;
    g_FrameTime = static_cast<float>(elapsed);
    g_TimeStart = g_TimeEnd;
    return true;
}
