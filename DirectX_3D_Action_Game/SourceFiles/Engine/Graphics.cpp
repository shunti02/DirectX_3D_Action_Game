#include "Engine/Graphics.h"
#include <iostream>
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_impl_win32.h"
#include "../ImGui/imgui_impl_dx11.h"

Graphics::Graphics() {}
Graphics::~Graphics() {
    if (ImGui::GetCurrentContext() != nullptr) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }
}

bool Graphics::Initialize(HWND hWnd, int width, int height)
{
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

    UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    hr = D3D11CreateDeviceAndSwapChain(
        NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 
        createDeviceFlags,
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

    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.AntialiasedLineEnable = FALSE;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.DepthBias = 0;
    rasterDesc.DepthBiasClamp = 0.0f;
    rasterDesc.DepthClipEnable = TRUE;
    rasterDesc.FillMode = D3D11_FILL_SOLID; 
    rasterDesc.FrontCounterClockwise = TRUE;
    rasterDesc.MultisampleEnable = FALSE;
    rasterDesc.ScissorEnable = FALSE;
    rasterDesc.SlopeScaledDepthBias = 0.0f;

    hr = pDevice->CreateRasterizerState(&rasterDesc, &pRasterizerState);
    if (FAILED(hr)) return false;

    pContext->RSSetState(pRasterizerState.Get());

    D3D11_TEXTURE2D_DESC depthBufferDesc = {};
    depthBufferDesc.Width = width;
    depthBufferDesc.Height = height;
    depthBufferDesc.MipLevels = 1;
    depthBufferDesc.ArraySize = 1;
    depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthBufferDesc.SampleDesc.Count = 1;
    depthBufferDesc.SampleDesc.Quality = 0;
    depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthBufferDesc.CPUAccessFlags = 0;
    depthBufferDesc.MiscFlags = 0;

    hr = pDevice->CreateTexture2D(&depthBufferDesc, NULL, &pDepthStencilBuffer);
    if (FAILED(hr)) return false;

    hr = pDevice->CreateDepthStencilView(pDepthStencilBuffer.Get(), NULL, &pDepthStencilView);
    if (FAILED(hr)) return false;

    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE; 
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS; 

    dsDesc.StencilEnable = FALSE;

    hr = pDevice->CreateDepthStencilState(&dsDesc, &pDepthStencilState);
    if (FAILED(hr)) return false;

    pContext->OMSetDepthStencilState(pDepthStencilState.Get(), 1);

    pContext->OMSetRenderTargets(1, pRenderTargetView.GetAddressOf(), pDepthStencilView.Get());
    if (!InitD2D(pSwapChain.Get())) {
        MessageBox(NULL, "Direct2D Init Failed", "Error", MB_OK);
        return false;
    }
    return true;
}

void Graphics::BeginFrame(float r, float g, float b, float a) {
    pContext->OMSetRenderTargets(1, pRenderTargetView.GetAddressOf(), pDepthStencilView.Get());
    const float color[] = { r, g, b, a };
    pContext->ClearRenderTargetView(pRenderTargetView.Get(), color);
    pContext->ClearDepthStencilView(pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void Graphics::EndFrame() {
    pSwapChain->Present(1, 0);
}
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

bool Graphics::InitD2D(IDXGISwapChain* swapChain)
{
    HRESULT hr;

    D2D1_FACTORY_OPTIONS options = {};
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,__uuidof(ID2D1Factory),&options,reinterpret_cast<void**>(pD2DFactory.GetAddressOf())
    );
    if (FAILED(hr)) return false;

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(pDWriteFactory.GetAddressOf()));
    if (FAILED(hr)) return false;

    ComPtr<IDXGISurface> pBackBuffer;
    hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (FAILED(hr)) return false;

    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );

    hr = pD2DFactory->CreateDxgiSurfaceRenderTarget(pBackBuffer.Get(), &props, &pD2DRenderTarget);
    if (FAILED(hr)) return false;

    hr = pDWriteFactory->CreateTextFormat(
        L"Meiryo", 
        NULL,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        24.0f,
        L"ja-jp",
        &pTextFormat
    );
    if (FAILED(hr)) return false;

    hr = pD2DRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pBrush);
    if (FAILED(hr)) return false;

    return true;
}

bool Graphics::CreateVertexShader(const std::wstring& filename, ID3D11VertexShader** ppVertexShader, ID3D11InputLayout** ppInputLayout)
{
    ComPtr<ID3DBlob> pVSBlob;
    if (!CompileShaderFromFile(filename, "main", "vs_5_0", &pVSBlob)) {
        MessageBoxW(NULL, L"Failed to compile vertex shader", filename.c_str(), MB_OK);
        return false;
    }
    HRESULT hr = pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, ppVertexShader);
    if (FAILED(hr)) return false;

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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

bool Graphics::CreateGeometryShader(const std::wstring& filename, ID3D11GeometryShader** ppGeometryShader)
{
    ComPtr<ID3DBlob> pGSBlob;
    if (!CompileShaderFromFile(filename, "main", "gs_5_0", &pGSBlob)) {
        MessageBoxW(NULL, L"Failed to compile geometry shader", filename.c_str(), MB_OK);
        return false;
    }

    HRESULT hr = pDevice->CreateGeometryShader(pGSBlob->GetBufferPointer(), pGSBlob->GetBufferSize(), nullptr, ppGeometryShader);
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
void Graphics::InitUI(HWND hWnd)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX11_Init(pDevice.Get(), pContext.Get());
}
void Graphics::BeginUI()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}
void Graphics::EndUI()
{
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void Graphics::BeginDraw2D()
{
    if (pD2DRenderTarget) {
        pD2DRenderTarget->BeginDraw();
    }
}

void Graphics::EndDraw2D()
{
    if (pD2DRenderTarget) {
        pD2DRenderTarget->EndDraw();
    }
}

void Graphics::DrawString(const std::wstring& text, float x, float y, float size, uint32_t color)
{
    if (!pD2DRenderTarget || !pDWriteFactory || !pBrush) return;

    ComPtr<IDWriteTextFormat> localFormat;
    HRESULT hr = pDWriteFactory->CreateTextFormat(
        L"Meiryo",
        NULL,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        size,
        L"ja-jp",
        &localFormat
    );
    if (FAILED(hr)) return;

    float a = ((color >> 24) & 0xFF) / 255.0f;
    float b = ((color >> 16) & 0xFF) / 255.0f;
    float g = ((color >> 8) & 0xFF) / 255.0f;
    float r = (color & 0xFF) / 255.0f;
    pBrush->SetColor(D2D1::ColorF(r, g, b, a));

    D2D1_RECT_F layoutRect = D2D1::RectF(x, y, x + 2000.0f, y + 2000.0f);

    pD2DRenderTarget->DrawText(
        text.c_str(),
        static_cast<UINT32>(text.length()),
        localFormat.Get(),
        layoutRect,
        pBrush.Get()
    );
}

void Graphics::DrawRect(float x, float y, float w, float h, uint32_t color)
{
    if (!pD2DRenderTarget) return;

    float a = ((color >> 24) & 0xFF) / 255.0f;
    float r = ((color >> 16) & 0xFF) / 255.0f;
    float g = ((color >> 8) & 0xFF) / 255.0f;
    float b = (color & 0xFF) / 255.0f;

    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    pD2DRenderTarget->CreateSolidColorBrush(D2D1::ColorF(r, g, b, a), &brush);

    if (brush) {
        D2D1_RECT_F rect = D2D1::RectF(x, y, x + w, y + h);
        pD2DRenderTarget->FillRectangle(rect, brush.Get());
    }
}

void Graphics::DrawRectOutline(float x, float y, float w, float h, float thickness, uint32_t color)
{
    if (!pD2DRenderTarget) return;

    float a = ((color >> 24) & 0xFF) / 255.0f;
    float r = ((color >> 16) & 0xFF) / 255.0f;
    float g = ((color >> 8) & 0xFF) / 255.0f;
    float b = (color & 0xFF) / 255.0f;

    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    pD2DRenderTarget->CreateSolidColorBrush(D2D1::ColorF(r, g, b, a), &brush);

    if (brush) {
        D2D1_RECT_F rect = D2D1::RectF(x, y, x + w, y + h);
        pD2DRenderTarget->DrawRectangle(rect, brush.Get(), thickness);
    }
}
void Graphics::FillRect(float x, float y, float w, float h, uint32_t color)
{
    DrawRect(x, y, w, h, color);
}