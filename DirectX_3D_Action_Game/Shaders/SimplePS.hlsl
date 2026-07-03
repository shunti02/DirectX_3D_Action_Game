struct VSOut
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
};

float4 main(VSOut input) : SV_Target
{
    float3 lightDir = normalize(float3(1.0f, -1.0f, 1.0f));
    float3 lightColor = float3(1.0f, 1.0f, 1.0f);
    float3 ambientColor = float3(0.3f, 0.3f, 0.3f);
    
    float diffuse = dot(input.normal, -lightDir);
    
    diffuse = saturate(diffuse);
    
    float3 finalLight = ambientColor + (lightColor * diffuse);
    
    float4 finalColor;
    finalColor.rgb = input.color.rgb * finalLight;
    finalColor.a = input.color.a;

    return finalColor;
}