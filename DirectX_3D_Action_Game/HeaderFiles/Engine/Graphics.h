/*===================================================================
//ファイル:Graphics.h
//制作日:2025/12/05
//概要:DirectX11のデバイス管理
//制作者:伊藤駿汰
//-------------------------------------------------------------------
//更新履歴:
//2025/12/05:新規作成
=====================================================================*/
#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>

//ライブラリのリンク
#pragma comment(lib, "d3d11.lib")

using Microsoft::WRL::ComPtr;

class Graphics {
public:
	Graphics();
	~Graphics();

	//初期化
	bool Initialize(HWND hWnd, int width, int height);

	void BeginFrame(float r, float g, float b, float a);

	void EndFrame();

	ID3D11Device* GetDevice() const { return pDevice.Get(); }
	ID3D11DeviceContext* GetContext() const { return pContext.Get(); }

private:
	ComPtr<ID3D11Device> pDevice;
	ComPtr<ID3D11DeviceContext> pContext;
	ComPtr<IDXGISwapChain> pSwapChain;
	ComPtr<ID3D11RenderTargetView> pRenderTargetView;
	ComPtr<ID3D11BlendState> pBlendState;
};
#endif //GRAPHICS_H