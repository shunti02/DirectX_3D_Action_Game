/*===================================================================
//ファイル:Main.cpp
//制作日:2025/12/05
//概要:アプリケーションのエントリーポイント、ウィンドウ生成とメインループを管理する
//制作者:伊藤駿汰
//-------------------------------------------------------------------
//更新履歴:
//2025/12/05:新規作成、ウィンドウ表示機能の実装
//2025/12/05:デバッグコンソールの実装
//2025/12/05:FPS管理実装
//2025/12/07:Mainの機能を移行Gameクラスを起動するのみに変更
=====================================================================*/
#include "App/Main.h"
#include "App/Game.h"
#include <iostream>
#include <cstdio>
#include <crtdbg.h>
#pragma comment(lib, "winmm.lib")

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
/*----------------------------------------------------------
//内部変数・定数
------------------------------------------------------------*/
namespace {
    std::unique_ptr<Game> g_Game;//ゲーム管理クラス
    //FPS管理用
    LARGE_INTEGER g_TimeFreq;//時間計測の周波数
    LARGE_INTEGER g_TimeStart;//計測開始時間
    LARGE_INTEGER g_TimeEnd;//計測終了時間
    float g_FrameTime = 0.0f;//1フレームにかかった時間（秒）
}

//プロトタイプ宣言
void InitFPS();
bool IsFrameReady();
/*-----------------------------------------------------------------------------
//WinMain
--------------------------------------------------------------------------------*/


//エントリーポイント
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	//メモリーリークチェックを有効化
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    //デバッグコンソールの割り当て
#ifdef _DEBUG
    if (AllocConsole()) {
        FILE* fp = nullptr;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
        freopen_s(&fp, "CONIN$", "r", stdin);
        std::cout.sync_with_stdio();
    }
#endif
    //タイマー精度向上
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
    //ウィンドウサイズの調整
    RECT rc = { 0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hWnd = CreateWindow(
        "DX11GameClass",Config::WINDOW_TITLE,WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,rc.right - rc.left, rc.bottom - rc.top,
        NULL, NULL, hInstance, NULL
    );

    if (!hWnd) return -1;

    // 4. ウィンドウ表示
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
            //フレーム更新タイミング待ち
            if (!IsFrameReady())continue;
            //Gameクラスの更新・描画
            Update(g_FrameTime);
            Draw();
        }
    }
    //終了処理
    UnInit();
    //タイマー設定を戻す
    timeEndPeriod(1);

#ifdef _DEBUG
    FreeConsole();
#endif

    return (int)msg.wParam;
}

//ウィンドウプロシージャ
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
   if(msg == WM_DESTROY)PostQuitMessage(0);
   if(msg == WM_KEYDOWN && wParam == VK_ESCAPE)PostQuitMessage(0);
   return DefWindowProc(hWnd, msg, wParam, lParam);
}

/*----------------------------------------------------------
//ラッパー関数群
-------------------------------------------------------------*/

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

//FPS制御関数の定義
void InitFPS()
{
    //パフォーマンスカウンタの周波数を取得
    QueryPerformanceFrequency(&g_TimeFreq);
    //現在のカウントを取得
    QueryPerformanceCounter(&g_TimeStart);
}

bool IsFrameReady()
{
    
    //今の時間を取得
    QueryPerformanceCounter(&g_TimeEnd);
    //経過時間を計算
    double elapsed = static_cast<double>(g_TimeEnd.QuadPart - g_TimeStart.QuadPart) / static_cast<double>(g_TimeFreq.QuadPart);
    //時間1/60
    double targetTime = 1.0 / Config::TARGET_FPS;
    //まだ時間が余っているなら待機
    if (elapsed < targetTime)return false;
    g_FrameTime = static_cast<float>(elapsed);
    //時間が経過したら計測開始時間を更新してtrueを返す
    g_TimeStart = g_TimeEnd;
    return true;
}

void DebugLog(const char* format, ...)
{
#ifdef _DEBUG
    char buffer[1024];
    va_list args;
    va_start(args, format);
    //フォーマットに従って文字列を生成
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end (args);

    //標準出力に出力
    std::cout << "[LOG]" << buffer << std::endl;

	//VisualStudioのデバッグ出力ウィンドウにも出力
    OutputDebugString(buffer);
    OutputDebugString("\n");
#endif
}
