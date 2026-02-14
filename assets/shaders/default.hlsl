float4 VSMain(float3 position : POSITION) : SV_Position
{
    return float4(position, 1.0f);
}

float4 PSMain() : SV_Target
{
    return float4(0.9f, 0.9f, 0.95f, 1.0f);
}
