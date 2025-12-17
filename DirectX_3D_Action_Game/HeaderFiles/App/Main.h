/*===================================================================
//ファイル:Main.h
//制作日:2025/12/05
//概要:Main.cppで使用する定数、グローバル変数、関数の宣言
//制作者:伊藤駿汰
//-------------------------------------------------------------------
//更新履歴:
//2025/12/05:新規作成
//2025/12/05:ECS及びゲーム開発に必要なユーティリティを統合
=====================================================================*/
#ifndef MAIN_H
#define MAIN_H
#include <Windows.h>
#include "Engine/Graphics.h"

//定数・マクロ定義
#define ASSET(path)"Asset/"path

//単位変換マクロ1.0f = 1m
#define METER(val) (val * 1.0f)
#define CMETER(val) (val * 0.01f)
//定数宣言
namespace Config {
	//画面サイズ
	constexpr int SCREEN_WIDTH = 1280;
	constexpr int SCREEN_HEIGHT = 720;
	//タイトル
	constexpr const char* WINDOW_TITLE = "DirectX 3D Action Game (ECS)";
	//FPS設定
	constexpr int TARGET_FPS = 60;

}

//関数プロトタイプ宣言

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//ゲーム制御関数
void Init(HWND hWnd);//初期化
void UnInit();//終了処理
void Update(float deltaTime);//更新処理
void Draw();//描画処理
//デバッグログ関数
void DebugLog(const char* format, ...);
#endif //MAIN_H
