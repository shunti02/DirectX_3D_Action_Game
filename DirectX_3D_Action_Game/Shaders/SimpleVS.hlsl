cbuffer CBuf : register(b0)
{
    matrix transform;
};

struct VSIn
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
};

struct VSOut
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
};

VSOut main(VSIn input)
{
    VSOut output;
    
    output.pos = mul(float4(input.pos, 1.0f), transform);
    
    output.normal = normalize(mul(float4(input.normal, 0.0f), transform).xyz);

    output.color = input.color;
    
    return output;
}