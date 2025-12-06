/*===================================================================
//ファイル:Graphics.cpp
//制作日:2025/12/05
//概要:Graphicsクラスの実装
//制作者:伊藤駿汰
//-------------------------------------------------------------------
//更新履歴:
//2025/12/05:新規作成
=====================================================================*/
#include "Engine/Graphics.h"

Graphics::Graphics()
{
}

Graphics::~Graphics()
{
}

bool Graphics::Initialize(HWND hWnd, int width, int height)
{
	HRESULT hr;
    // デバイスとスワップチェーンの作成
	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferCount = 1;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
   
    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL featureLevel;

    hr = D3D11CreateDeviceAndSwapChain(
        NULL,D3D_DRIVER_TYPE_HARDWARE,NULL,0,
        featureLevels,1,D3D11_SDK_VERSION,
        &sd,&pSwapChain,&pDevice,&featureLevel,&pContext
    );

    if (FAILED(hr))return false;

    // レンダーターゲットビュー（描画先）の作成
    ComPtr<ID3D11Texture2D> pBackBuffer;
    hr = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
    if (FAILED(hr))  return false;

    hr = pDevice->CreateRenderTargetView(pBackBuffer.Get(), NULL, &pRenderTargetView);
    if (FAILED(hr))return false;

    // ビューポート（描画範囲）の設定
    D3D11_VIEWPORT vp;
    vp.Width = (float)width;
    vp.Height = (float)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    pContext->RSSetViewports(1, &vp);

    //ブレンドステートの作成と設定
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;//ブレンドの有効
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;//元の色＊アルファ値
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;//先の色＊（1 - アルファ値）
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;//足し合わせる
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = pDevice->CreateBlendState(&blendDesc, &pBlendState);
    if (FAILED(hr))return false;

    //作成したステートをパイプラインにセット
    float blendFactor[4] = { 0.0f,0.0f,0.0f,0.0f };
    pContext->OMSetBlendState(pBlendState.Get(), blendFactor, 0xffffffff);

    return true;
}

void Graphics::BeginFrame(float r, float g, float b, float a)
{
    // 指定色で画面をクリア
    const float color[] = { r, g, b, a };
    pContext->ClearRenderTargetView(pRenderTargetView.Get(), color);
}

void Graphics::EndFrame()
{
    // 画面更新 (VSync有効)
    pSwapChain->Present(1, 0);
}
