#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <d3dcompiler.h>
#include <string>
#include <vector>
#include "Vertex.h"
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
    bool CreateVertexShader(const std::wstring& filename, ID3D11VertexShader** ppVertexShader, ID3D11InputLayout** ppInputLayout);
    bool CreatePixelShader(const std::wstring& filename, ID3D11PixelShader** ppPixelShader);
    bool CreateGeometryShader(const std::wstring& filename, ID3D11GeometryShader** ppGeometryShader);
    bool CreateConstantBuffer(UINT size, ID3D11Buffer** ppBuffer);
    bool CreateVertexBuffer(const std::vector<Vertex>& vertices, ID3D11Buffer** ppBuffer);
    bool CreateIndexBuffer(const std::vector<UINT>& indices, ID3D11Buffer** ppBuffer);

    void InitUI(HWND hWnd);
	void BeginUI();
	void EndUI();

    void BeginDraw2D();
    void EndDraw2D();
    void DrawString(const std::wstring& text, float x, float y, float size, uint32_t color);
    void DrawRect(float x, float y, float w, float h, uint32_t color);
    void FillRect(float x, float y, float w, float h, uint32_t color);

    bool CompileShaderFromFile(const std::wstring& filename, const std::string& entryPoint, const std::string& shaderModel, ID3DBlob** ppBlobOut);

    void DrawRectOutline(float x, float y, float w, float h, float thickness, uint32_t color);
private:
    ComPtr<ID3D11Device> pDevice;
    ComPtr<ID3D11DeviceContext> pContext;
    ComPtr<IDXGISwapChain> pSwapChain;
    ComPtr<ID3D11RenderTargetView> pRenderTargetView;
    ComPtr<ID3D11BlendState> pBlendState;
    ComPtr<ID3D11RasterizerState> pRasterizerState;
    ComPtr<ID3D11Texture2D> pDepthStencilBuffer;
    ComPtr<ID3D11DepthStencilView> pDepthStencilView;
    ComPtr<ID3D11DepthStencilState> pDepthStencilState;
    ComPtr<ID2D1Factory> pD2DFactory;
    ComPtr<IDWriteFactory> pDWriteFactory;
    ComPtr<ID2D1RenderTarget> pD2DRenderTarget;
    ComPtr<IDWriteTextFormat> pTextFormat;
    ComPtr<ID2D1SolidColorBrush> pBrush;

    
    bool InitD2D(IDXGISwapChain* swapChain);
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_pD2DSolidBrush;
};
#endif