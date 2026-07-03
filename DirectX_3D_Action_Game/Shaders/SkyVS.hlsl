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
    
    output.TexCoord = input.Position.xyz;
    
    matrix viewNoTranslate = View;
    viewNoTranslate[3][0] = 0.0f;
    viewNoTranslate[3][1] = 0.0f;
    viewNoTranslate[3][2] = 0.0f;
    
    float4 pos = mul(input.Position, World);
    pos = mul(pos, viewNoTranslate);
    pos = mul(pos, Projection);
    
    output.Position = pos.xyww;

    return output;
}