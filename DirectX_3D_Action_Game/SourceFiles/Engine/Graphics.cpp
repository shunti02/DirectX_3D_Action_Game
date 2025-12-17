/*===================================================================
//ファイル:Graphics.cpp
//概要:Graphicsクラスの実装 (シェーダー読み込み機能追加版)
=====================================================================*/
#include "Engine/Graphics.h"
#include <iostream> // デバッグ出力用
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_impl_win32.h"
#include "../ImGui/imgui_impl_dx11.h"

Graphics::Graphics() {}
Graphics::~Graphics() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

bool Graphics::Initialize(HWND hWnd, int width, int height)
{
    // ... (既存の初期化コードは変更なし。そのまま維持してください) ...
    // ※もし全文置き換える場合は、以前のInitializeの中身をここにコピーしてください。
    //   ここでは省略せずに書いておきますが、以前と同じ内容です。

    HRESULT hr;
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
        NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0,
        featureLevels, 1, D3D11_SDK_VERSION,
        &sd, &pSwapChain, &pDevice, &featureLevel, &pContext
    );
    if (FAILED(hr)) return false;

    ComPtr<ID3D11Texture2D> pBackBuffer;
    hr = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
    if (FAILED(hr)) return false;

    hr = pDevice->CreateRenderTargetView(pBackBuffer.Get(), NULL, &pRenderTargetView);
    if (FAILED(hr)) return false;

    D3D11_VIEWPORT vp;
    vp.Width = (float)width;
    vp.Height = (float)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    pContext->RSSetViewports(1, &vp);

    // ブレンドステート作成（アルファブレンド有効）
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = pDevice->CreateBlendState(&blendDesc, &pBlendState);
    if (FAILED(hr)) return false;

    float blendFactor[4] = { 0.0f,0.0f,0.0f,0.0f };
    pContext->OMSetBlendState(pBlendState.Get(), blendFactor, 0xffffffff);

    // ★追加: ラスタライザーステートの作成（両面描画設定）
    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.AntialiasedLineEnable = FALSE;
    rasterDesc.CullMode = D3D11_CULL_BACK; // ★重要: "NONE" にすると裏面も描画される
    rasterDesc.DepthBias = 0;
    rasterDesc.DepthBiasClamp = 0.0f;
    rasterDesc.DepthClipEnable = TRUE;
    rasterDesc.FillMode = D3D11_FILL_SOLID; // ポリゴンの中身を塗りつぶす
    rasterDesc.FrontCounterClockwise = TRUE;
    rasterDesc.MultisampleEnable = FALSE;
    rasterDesc.ScissorEnable = FALSE;
    rasterDesc.SlopeScaledDepthBias = 0.0f;

    hr = pDevice->CreateRasterizerState(&rasterDesc, &pRasterizerState);
    if (FAILED(hr)) return false;

    // 作成したステートをセット
    pContext->RSSetState(pRasterizerState.Get());

    // 1. 深度ステンシルテクスチャの作成
    D3D11_TEXTURE2D_DESC depthBufferDesc = {};
    depthBufferDesc.Width = width;
    depthBufferDesc.Height = height;
    depthBufferDesc.MipLevels = 1;
    depthBufferDesc.ArraySize = 1;
    depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 深度24bit, ステンシル8bit
    depthBufferDesc.SampleDesc.Count = 1;
    depthBufferDesc.SampleDesc.Quality = 0;
    depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthBufferDesc.CPUAccessFlags = 0;
    depthBufferDesc.MiscFlags = 0;

    hr = pDevice->CreateTexture2D(&depthBufferDesc, NULL, &pDepthStencilBuffer);
    if (FAILED(hr)) return false;

    // 2. 深度ステンシルビューの作成
    hr = pDevice->CreateDepthStencilView(pDepthStencilBuffer.Get(), NULL, &pDepthStencilView);
    if (FAILED(hr)) return false;

    // 3. 深度ステンシルステートの作成（深度テストを有効にする設定）
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE; // ★重要: 深度テストON
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; // 書き込み許可
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS; // 手前にあるものを描画

    dsDesc.StencilEnable = FALSE;

    hr = pDevice->CreateDepthStencilState(&dsDesc, &pDepthStencilState);
    if (FAILED(hr)) return false;

    // ステートをセット
    pContext->OMSetDepthStencilState(pDepthStencilState.Get(), 1);

    // 4. レンダーターゲットと深度ビューを紐づけてセット
    pContext->OMSetRenderTargets(1, pRenderTargetView.GetAddressOf(), pDepthStencilView.Get());

    return true;
}

void Graphics::BeginFrame(float r, float g, float b, float a) {
    pContext->OMSetRenderTargets(1, pRenderTargetView.GetAddressOf(), pDepthStencilView.Get());
    const float color[] = { r, g, b, a };
    pContext->ClearRenderTargetView(pRenderTargetView.Get(), color);
    //深度バッファクリア (1.0f = 最も奥、で初期化)
    pContext->ClearDepthStencilView(pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void Graphics::EndFrame() {
    pSwapChain->Present(1, 0);
}

// シェーダーコンパイル用ヘルパー関数
bool Graphics::CompileShaderFromFile(const std::wstring& filename, const std::string& entryPoint, const std::string& shaderModel, ID3DBlob** ppBlobOut)
{
    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    dwShaderFlags |= D3DCOMPILE_DEBUG;
    dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> pErrorBlob;
    HRESULT hr = D3DCompileFromFile(
        filename.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryPoint.c_str(), shaderModel.c_str(),
        dwShaderFlags, 0, ppBlobOut, &pErrorBlob
    );

    if (FAILED(hr)) {
        if (pErrorBlob) {
            OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
            std::cout << "Shader Compile Error: " << reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()) << std::endl;
        }
        return false;
    }
    return true;
}

// 頂点シェーダー作成
bool Graphics::CreateVertexShader(const std::wstring& filename, ID3D11VertexShader** ppVertexShader, ID3D11InputLayout** ppInputLayout)
{
    ComPtr<ID3DBlob> pVSBlob;
    // hlslファイルをコンパイル (エントリーポイント:main, モデル:vs_5_0)
    if (!CompileShaderFromFile(filename, "main", "vs_5_0", &pVSBlob)) {
        MessageBoxW(NULL, L"Failed to compile vertex shader", filename.c_str(), MB_OK);
        return false;
    }

    // シェーダーオブジェクト作成
    HRESULT hr = pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, ppVertexShader);
    if (FAILED(hr)) return false;

    // 入力レイアウト（頂点データの形式）の定義
    // Vertex.h の構造と合わせる必要があります
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = pDevice->CreateInputLayout(
        layout, ARRAYSIZE(layout),
        pVSBlob->GetBufferPointer(),
        pVSBlob->GetBufferSize(),
        ppInputLayout
    );
    if (FAILED(hr)) return false;

    return true;
}

// ピクセルシェーダー作成
bool Graphics::CreatePixelShader(const std::wstring& filename, ID3D11PixelShader** ppPixelShader)
{
    ComPtr<ID3DBlob> pPSBlob;
    if (!CompileShaderFromFile(filename, "main", "ps_5_0", &pPSBlob)) {
        MessageBoxW(NULL, L"Failed to compile pixel shader", filename.c_str(), MB_OK);
        return false;
    }

    HRESULT hr = pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, ppPixelShader);
    if (FAILED(hr)) return false;

    return true;
}

bool Graphics::CreateConstantBuffer(UINT size, ID3D11Buffer** ppBuffer)
{
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = size;
    if (bd.ByteWidth % 16 != 0) bd.ByteWidth += 16 - (bd.ByteWidth % 16);

    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;

    HRESULT hr = pDevice->CreateBuffer(&bd, nullptr, ppBuffer);
    return SUCCEEDED(hr);
}

// 頂点バッファ作成
bool Graphics::CreateVertexBuffer(const std::vector<Vertex>& vertices, ID3D11Buffer** ppBuffer)
{
    if (vertices.empty()) return false;

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = static_cast<UINT>(sizeof(Vertex) * vertices.size());
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices.data();

    HRESULT hr = pDevice->CreateBuffer(&bd, &initData, ppBuffer);
    return (SUCCEEDED(hr));
}

bool Graphics::CreateIndexBuffer(const std::vector<UINT>& indices, ID3D11Buffer** ppBuffer)
{
    if (indices.empty()) return false;

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = static_cast<UINT>(sizeof(UINT) * indices.size());
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = indices.data();

    HRESULT hr = pDevice->CreateBuffer(&bd, &initData, ppBuffer);
    return SUCCEEDED(hr);
}

// ---------------------------------------------------------
// ImGui管理関数の実装
// ---------------------------------------------------------
void Graphics::InitUI(HWND hWnd)
{
    // ImGuiコンテキスト作成
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    // ImGuiスタイル設定
    ImGui::StyleColorsDark();
    // Win32 + DirectX11用の初期化
    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX11_Init(pDevice.Get(), pContext.Get());
}
void Graphics::BeginUI()
{
    // ImGuiフレーム開始
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}
void Graphics::EndUI()
{
    // ImGuiフレーム終了・描画
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}