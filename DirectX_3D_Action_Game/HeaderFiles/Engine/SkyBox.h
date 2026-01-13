/*===================================================================
// ファイル: SkyBox.h
// 概要: 宇宙空間を擬似生成して描画するクラス
=====================================================================*/
#pragma once
#include "Graphics.h"
#include <vector>
#include <DirectXMath.h>

using namespace DirectX;

class SkyBox {
public:
	SkyBox();
	~SkyBox();

	//初期化
	bool Initialize(Graphics* pGraphics);
	//描画
	void Draw(Graphics* pGraphics, const XMMATRIX& view, const XMMATRIX& proj);

private:
	//内部リソース
    // 内部リソース
    ComPtr<ID3D11Buffer> pVertexBuffer;
    ComPtr<ID3D11Buffer> pIndexBuffer;
    ComPtr<ID3D11VertexShader> pVertexShader;
    ComPtr<ID3D11PixelShader> pPixelShader;
    ComPtr<ID3D11InputLayout> pInputLayout;
    ComPtr<ID3D11Buffer> pConstantBuffer;

    // テクスチャ関連
    ComPtr<ID3D11ShaderResourceView> pTextureView; // キューブマップ
    ComPtr<ID3D11SamplerState> pSamplerState;
    ComPtr<ID3D11DepthStencilState> pSkyDepthState; // SkyBox用の特殊な深度設定

    UINT indexCount = 0;

    // 宇宙テクスチャ生成関数
    bool CreateProceduralSpaceTexture(ID3D11Device* pDevice);

    // 立方体メッシュ生成
    bool CreateBoxMesh(Graphics* pGraphics);
};
