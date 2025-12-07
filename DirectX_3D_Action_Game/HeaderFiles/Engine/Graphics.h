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

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

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

    bool CreateConstantBuffer(UINT size, ID3D11Buffer** ppBuffer);
    // ★追加: バッファ作成
    bool CreateVertexBuffer(const std::vector<Vertex>& vertices, ID3D11Buffer** ppBuffer);
    bool CreateIndexBuffer(const std::vector<UINT>& indices, ID3D11Buffer** ppBuffer);

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

    // 内部ヘルパー: シェーダーコンパイル
    bool CompileShaderFromFile(const std::wstring& filename, const std::string& entryPoint, const std::string& shaderModel, ID3DBlob** ppBlobOut);
};
#endif //GRAPHICS_H