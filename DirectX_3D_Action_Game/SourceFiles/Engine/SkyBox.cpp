/*===================================================================
// ファイル: SkyBox.cpp
// 概要: SkyBoxクラスの実装 (宇宙テクスチャ生成付き)
=====================================================================*/
#include "Engine/SkyBox.h"
#include <random> // 乱数用

struct SkyBoxConstantBuffer {
    XMMATRIX World;
    XMMATRIX View;
    XMMATRIX Projection;
};

SkyBox::SkyBox() {}
SkyBox::~SkyBox() {}

bool SkyBox::Initialize(Graphics* pGraphics) {
    //ID3D11Device* pDevice = pGraphics->GetDevice();
    //HRESULT hr;

    //// 1. 立方体メッシュの作成
    //if (!CreateBoxMesh(pGraphics)) return false;

    //// 2. シェーダーの作成 (★修正: InputLayoutをSkyBox専用にするため手動作成)
    //ComPtr<ID3DBlob> pVSBlob;
    //ComPtr<ID3DBlob> pErrorBlob;

    //// 頂点シェーダーコンパイル
    //// ※ファイルの場所が間違っているとここで失敗します
    //hr = D3DCompileFromFile(
    //    L"Shaders/SkyVS.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
    //    "main", "vs_5_0",
    //    D3DCOMPILE_ENABLE_STRICTNESS, 0,
    //    &pVSBlob, &pErrorBlob
    //);

    //if (FAILED(hr)) {
    //    if (pErrorBlob) {
    //        OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
    //    }
    //    return false;
    //}

    //hr = pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &pVertexShader);
    //if (FAILED(hr)) return false;

    //// ★重要: SkyBox用のInputLayout (COLORを含まず、POSITIONのみ)
    //D3D11_INPUT_ELEMENT_DESC layout[] = {
    //    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    //};

    //hr = pDevice->CreateInputLayout(
    //    layout, ARRAYSIZE(layout),
    //    pVSBlob->GetBufferPointer(),
    //    pVSBlob->GetBufferSize(),
    //    &pInputLayout
    //);
    //if (FAILED(hr)) return false;

    //// ピクセルシェーダー (こちらはGraphicsの関数を使ってOK)
    //if (!pGraphics->CreatePixelShader(L"Shaders/SkyPS.hlsl", &pPixelShader)) return false;

    //// 3. 定数バッファ作成
    //if (!pGraphics->CreateConstantBuffer(sizeof(SkyBoxConstantBuffer), &pConstantBuffer)) return false;

    //// 4. 宇宙テクスチャの生成
    //if (!CreateProceduralSpaceTexture(pDevice)) return false;

    //// 5. サンプラーステート作成
    //D3D11_SAMPLER_DESC sampDesc = {};
    //sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    //sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    //sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    //sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    //sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    //hr = pDevice->CreateSamplerState(&sampDesc, &pSamplerState);
    //if (FAILED(hr)) return false;

    //// 6. SkyBox専用の深度ステート作成
    //D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    //dsDesc.DepthEnable = TRUE;
    //dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    //dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    //hr = pDevice->CreateDepthStencilState(&dsDesc, &pSkyDepthState);
    //if (FAILED(hr)) return false;

    //return true;
    ID3D11Device* pDevice = pGraphics->GetDevice();
    HRESULT hr;

    // 1. メッシュ作成チェック
    if (!CreateBoxMesh(pGraphics)) {
        MessageBox(NULL, "CreateBoxMesh Failed.", "SkyBox Error", MB_OK);
        return false;
    }

    // 2. シェーダーコンパイル (詳細エラー表示付き)
    ComPtr<ID3DBlob> pVSBlob;
    ComPtr<ID3DBlob> pErrorBlob;

    // ファイルパスの確認
    // ※L"Shaders/SkyVS.hlsl" で見つからない場合が多い
    LPCWSTR vsFilename = L"Shaders/SkyVS.hlsl";

    hr = D3DCompileFromFile(
        vsFilename, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "main", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0,
        &pVSBlob, &pErrorBlob
    );

    if (FAILED(hr)) {
        std::string errorMsg = "Failed to compile Vertex Shader: ";
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
            errorMsg += "File not found (Path is wrong).";
        }
        else if (pErrorBlob) {
            errorMsg += (char*)pErrorBlob->GetBufferPointer();
        }
        else {
            errorMsg += "Unknown Error (Check HRESULT).";
        }
        MessageBoxA(NULL, errorMsg.c_str(), "SkyBox Error", MB_OK);
        return false;
    }

    hr = pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &pVertexShader);
    if (FAILED(hr)) return false;

    // InputLayout
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &pInputLayout);
    if (FAILED(hr)) {
        MessageBox(NULL, "CreateInputLayout Failed.", "SkyBox Error", MB_OK);
        return false;
    }

    // Pixel Shader
    if (!pGraphics->CreatePixelShader(L"Shaders/SkyPS.hlsl", &pPixelShader)) {
        // CreatePixelShader内部でMessageBoxが出ない場合のためにここでも出す
        MessageBox(NULL, "CreatePixelShader Failed (Maybe File not found).", "SkyBox Error", MB_OK);
        return false;
    }

    // 3. Constant Buffer
    if (!pGraphics->CreateConstantBuffer(sizeof(SkyBoxConstantBuffer), &pConstantBuffer)) {
        MessageBox(NULL, "CreateConstantBuffer Failed.", "SkyBox Error", MB_OK);
        return false;
    }

    // 4. Texture
    if (!CreateProceduralSpaceTexture(pDevice)) {
        MessageBox(NULL, "CreateProceduralSpaceTexture Failed.", "SkyBox Error", MB_OK);
        return false;
    }

    // 5. Sampler
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    hr = pDevice->CreateSamplerState(&sampDesc, &pSamplerState);
    if (FAILED(hr)) return false;

    // 6. Depth State
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    hr = pDevice->CreateDepthStencilState(&dsDesc, &pSkyDepthState);
    if (FAILED(hr)) return false;

    return true;
}

// 宇宙テクスチャの生成
bool SkyBox::CreateProceduralSpaceTexture(ID3D11Device* pDevice) {
    const int size = 512; // テクスチャサイズ (512x512)
    const int faces = 6;  // キューブマップは6面

    // ピクセルデータ作成 (RGBA各1バイト)
    // 黒背景にランダムで白を入れる
    std::vector<uint32_t> pixels(size * size * faces);

    std::mt19937 rng(12345); // 乱数生成器
    std::uniform_int_distribution<int> dist(0, 1000); // 確率調整用

    for (int i = 0; i < size * size * faces; ++i) {
        // 基本は真っ黒 (Alpha=255)
        uint32_t color = 0xFF000000;

        // 0.2%の確率で星を描画
        if (dist(rng) < 2) {
            // ランダムな明るさの白
            int brightness = 100 + (dist(rng) % 155);
            uint8_t b = static_cast<uint8_t>(brightness);
            // ABGR順 (Little Endian) -> 0xAABBGGRR
            color = 0xFF000000 | (b << 16) | (b << 8) | b;
        }
        pixels[i] = color;
    }

    // Direct3D用テクスチャ記述子
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = size;
    texDesc.Height = size;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 6; // 6面
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_IMMUTABLE;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE; // ★キューブマップ指定

    // 初期データ (6面分)
    D3D11_SUBRESOURCE_DATA pData[6];
    for (int i = 0; i < 6; ++i) {
        pData[i].pSysMem = &pixels[i * size * size];
        pData[i].SysMemPitch = size * 4; // 1行あたりのバイト数
        pData[i].SysMemSlicePitch = 0;
    }

    ComPtr<ID3D11Texture2D> pTexture;
    HRESULT hr = pDevice->CreateTexture2D(&texDesc, pData, &pTexture);
    if (FAILED(hr)) return false;

    // シェーダーリソースビュー作成
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE; // ★キューブマップとして見る
    srvDesc.TextureCube.MostDetailedMip = 0;
    srvDesc.TextureCube.MipLevels = 1;

    hr = pDevice->CreateShaderResourceView(pTexture.Get(), &srvDesc, &pTextureView);
    if (FAILED(hr)) return false;

    return true;
}

// ボックスメッシュ作成
bool SkyBox::CreateBoxMesh(Graphics* pGraphics) {
    // SkyBox用の単純な頂点 (座標のみ)
    struct SkyVertex {
        float x, y, z;
    };

    // 単位立方体の頂点 (サイズはシェーダーで無限遠に飛ばすので適当でよい)
    std::vector<SkyVertex> vertices = {
        {-1.0f,  1.0f, -1.0f}, { 1.0f,  1.0f, -1.0f}, { 1.0f,  1.0f,  1.0f}, {-1.0f,  1.0f,  1.0f}, // Top
        {-1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f,  1.0f}, {-1.0f, -1.0f,  1.0f}, // Bottom
    };

    // インデックス (三角形リスト)
    std::vector<UINT> indices = {
        3,1,0, 2,1,3, // Top
        6,4,5, 7,4,6, // Bottom
        3,0,4, 7,3,4, // Left
        1,2,5, 2,6,5, // Right
        2,3,7, 6,2,7, // Front
        0,1,5, 4,0,5  // Back
    };

    indexCount = static_cast<UINT>(indices.size());

    // バッファ作成
    // ※GraphicsクラスのCreateVertexBufferはVertex型固定なので、
    //   ここでは個別に作成する (またはGraphicsを拡張してもよいが今回は直接書く)
    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.ByteWidth = sizeof(SkyVertex) * (UINT)vertices.size();
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vinit = {};
    vinit.pSysMem = vertices.data();
    if (FAILED(pGraphics->GetDevice()->CreateBuffer(&vbd, &vinit, &pVertexBuffer))) return false;

    if (!pGraphics->CreateIndexBuffer(indices, &pIndexBuffer)) return false;

    return true;
}

void SkyBox::Draw(Graphics* pGraphics, const XMMATRIX& view, const XMMATRIX& proj) {
    if (!pConstantBuffer || !pVertexBuffer) return;
    auto context = pGraphics->GetContext();

    // 1. ステート変更 (SkyBox用)
    ComPtr<ID3D11DepthStencilState> prevDepthState;
    UINT prevRef;
    context->OMGetDepthStencilState(&prevDepthState, &prevRef);
    context->OMSetDepthStencilState(pSkyDepthState.Get(), 0);

    // 2. 定数バッファ更新
    SkyBoxConstantBuffer cb;
    cb.World = XMMatrixIdentity();
    cb.View = XMMatrixTranspose(view);
    cb.Projection = XMMatrixTranspose(proj);
    context->UpdateSubresource(pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

    // 3. パイプライン設定
    UINT stride = sizeof(float) * 3; // SkyVertex
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, pVertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    context->IASetInputLayout(pInputLayout.Get());
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->VSSetShader(pVertexShader.Get(), nullptr, 0);
    context->VSSetConstantBuffers(0, 1, pConstantBuffer.GetAddressOf());

    context->PSSetShader(pPixelShader.Get(), nullptr, 0);
    context->PSSetShaderResources(0, 1, pTextureView.GetAddressOf());
    context->PSSetSamplers(0, 1, pSamplerState.GetAddressOf());

    // 4. 描画
    context->DrawIndexed(indexCount, 0, 0);

    // 5. ステート復元 (これを忘れると他の描画がおかしくなる)
    context->OMSetDepthStencilState(prevDepthState.Get(), prevRef);
}