/*===================================================================
// ファイル: SkyPS.hlsl
// 概要: SkyBox用ピクセルシェーダー
=====================================================================*/
TextureCube g_SkyCube : register(t0);
SamplerState g_Sampler : register(s0);

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float3 TexCoord : TEXCOORD0;
};

float4 main(PS_INPUT input) :SV_TARGET
{
   // キューブマップからサンプリング
    return g_SkyCube.Sample(g_Sampler, input.TexCoord);
}