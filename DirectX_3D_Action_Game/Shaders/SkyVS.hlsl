/*===================================================================
// ファイル: SkyVS.hlsl
// 概要: SkyBox用頂点シェーダー
=====================================================================*/

cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
}

struct VS_INPUT
{
    float4 Position : POSITION;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float3 TexCoord : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    // 1. 頂点座標をそのままテクスチャサンプリング方向として使う
    output.TexCoord = input.Position.xyz;

    // 2. カメラの「平行移動」成分を無視して回転のみ適用するための処理
    // ビュー行列の平行移動成分を削除（SkyBoxは常にカメラについてくるため）
    matrix viewNoTranslate = View;
    viewNoTranslate[3][0] = 0.0f;
    viewNoTranslate[3][1] = 0.0f;
    viewNoTranslate[3][2] = 0.0f;

    // 3. 座標変換
    float4 pos = mul(input.Position, World); // Worldは基本単位行列
    pos = mul(pos, viewNoTranslate);
    pos = mul(pos, Projection);

    // 4. 重要: 常に深度バッファの「一番奥 (Z=1.0)」に描画させるトリック
    // XYWWとすることで、透視除算後のZ値が W/W = 1.0 になる
    output.Position = pos.xyww;

    return output;
}