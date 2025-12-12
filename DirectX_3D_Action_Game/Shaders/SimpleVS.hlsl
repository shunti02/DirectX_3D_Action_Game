/*===================================================================
//ファイル:SimpleVS.hlsl
//概要:基本的な頂点シェーダー（位置と色をそのまま渡す）
=====================================================================*/

// 定数バッファ（C++から行列を受け取る場所）
cbuffer CBuf : register(b0)
{
    matrix transform; // World * View * Projection 行列
};

// 入力データ（C++のVertex構造体と対応）
struct VSIn
{
    float3 pos : POSITION;
    float4 color : COLOR;
};

// 出力データ（ピクセルシェーダーへ渡す）
struct VSOut
{
    float4 pos : SV_POSITION; // スクリーン上の位置
    float4 color : COLOR; // 色
};

// メイン関数
VSOut main(VSIn input)
{
    VSOut output;
    
    // 座標変換（今のところはまだ行列を使わず、そのまま出力するテスト用）
    output.pos = mul(float4(input.pos, 1.0f), transform); 
    
    //// ★最初は「画面に映る範囲(-1.0 ~ 1.0)」に直接頂点を置くテストをするため、変換なしにします
    //output.pos = float4(input.pos, 1.0f);

    output.color = input.color;
    
    return output;
}