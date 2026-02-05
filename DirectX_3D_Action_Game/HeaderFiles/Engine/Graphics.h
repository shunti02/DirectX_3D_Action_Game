/*===================================================================
//ファイル:Graphics.h
//概要:DirectX11デバイス管理 + シェーダー・バッファ管理機能の追加
=====================================================================*/
#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <d3dcompiler.h> // シェーダーコンパイラ
#include <string>
#include <vector>
#include "Vertex.h"      // 頂点定義
#include <d2d1.h>
#include <dwrite.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

using Microsoft::WRL::ComPtr;

class Graphics {
public:
    Graphics();
    ~Graphics();

    bool Initialize(HWND hWnd, int width, int height);
    void BeginFrame(float r, float g, float b, float a);
    void EndFrame();

    ID3D11Device* GetDevice() const { return pDevice.Get(); }
    ID3D11DeviceContext* GetContext() const { return pContext.Get(); }

    // ★追加: シェーダー作成・設定
    bool CreateVertexShader(const std::wstring& filename, ID3D11VertexShader** ppVertexShader, ID3D11InputLayout** ppInputLayout);
    bool CreatePixelShader(const std::wstring& filename, ID3D11PixelShader** ppPixelShader);
    // ★追加: ジオメトリシェーダ作成
    bool CreateGeometryShader(const std::wstring& filename, ID3D11GeometryShader** ppGeometryShader);
    bool CreateConstantBuffer(UINT size, ID3D11Buffer** ppBuffer);
    // ★追加: バッファ作成
    bool CreateVertexBuffer(const std::vector<Vertex>& vertices, ID3D11Buffer** ppBuffer);
    bool CreateIndexBuffer(const std::vector<UINT>& indices, ID3D11Buffer** ppBuffer);

    void InitUI(HWND hWnd);
	void BeginUI();
	void EndUI();

    void BeginDraw2D();
    void EndDraw2D();
	// 文字列描画
    void DrawString(const std::wstring& text, float x, float y, float size, uint32_t color);
    // ★追加: 図形描画用
    void DrawRect(float x, float y, float w, float h, uint32_t color); // 枠線
    void FillRect(float x, float y, float w, float h, uint32_t color); // 塗りつぶし

    // 内部ヘルパー: シェーダーコンパイル
    bool CompileShaderFromFile(const std::wstring& filename, const std::string& entryPoint, const std::string& shaderModel, ID3DBlob** ppBlobOut);

    // ★追加: 枠線描画
    void DrawRectOutline(float x, float y, float w, float h, float thickness, uint32_t color);
private:
    ComPtr<ID3D11Device> pDevice;
    ComPtr<ID3D11DeviceContext> pContext;
    ComPtr<IDXGISwapChain> pSwapChain;
    ComPtr<ID3D11RenderTargetView> pRenderTargetView;
    ComPtr<ID3D11BlendState> pBlendState;
    ComPtr<ID3D11RasterizerState> pRasterizerState;
    ComPtr<ID3D11Texture2D> pDepthStencilBuffer;      // 深度バッファ本体
    ComPtr<ID3D11DepthStencilView> pDepthStencilView; // 深度バッファのビュー
    ComPtr<ID3D11DepthStencilState> pDepthStencilState; // 深度テストの設定
    ComPtr<ID2D1Factory> pD2DFactory;
    ComPtr<IDWriteFactory> pDWriteFactory;
    ComPtr<ID2D1RenderTarget> pD2DRenderTarget;
    ComPtr<IDWriteTextFormat> pTextFormat;
    ComPtr<ID2D1SolidColorBrush> pBrush;

    
    bool InitD2D(IDXGISwapChain* swapChain);
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_pD2DSolidBrush;
};
#endif //GRAPHICS_H