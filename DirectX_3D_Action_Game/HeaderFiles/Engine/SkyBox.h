#pragma once
#include "Graphics.h"
#include <vector>
#include <DirectXMath.h>

using namespace DirectX;

class SkyBox {
public:
	SkyBox();
	~SkyBox();
	bool Initialize(Graphics* pGraphics);
	void Draw(Graphics* pGraphics, const XMMATRIX& view, const XMMATRIX& proj);

private:
    ComPtr<ID3D11Buffer> pVertexBuffer;
    ComPtr<ID3D11Buffer> pIndexBuffer;
    ComPtr<ID3D11VertexShader> pVertexShader;
    ComPtr<ID3D11PixelShader> pPixelShader;
    ComPtr<ID3D11InputLayout> pInputLayout;
    ComPtr<ID3D11Buffer> pConstantBuffer;
    ComPtr<ID3D11ShaderResourceView> pTextureView;
    ComPtr<ID3D11SamplerState> pSamplerState;
    ComPtr<ID3D11DepthStencilState> pSkyDepthState;

    UINT indexCount = 0;

    bool CreateProceduralSpaceTexture(ID3D11Device* pDevice);

    bool CreateBoxMesh(Graphics* pGraphics);
};
