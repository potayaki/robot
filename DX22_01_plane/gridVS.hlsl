cbuffer WorldBuffer : register(b0) {
    matrix World;
}
cbuffer ViewBuffer : register(b1) {
    matrix View;
}
cbuffer ProjectionBuffer : register(b2) {
    matrix Projection;
}

struct VS_IN {
    float4 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

struct VS_OUT {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};

VS_OUT main(VS_IN input) {
    VS_OUT output;
    /*
    
    // 3Dカメラの座標変換
    float4 wPos = mul(World, input.pos);
    float4 vPos = mul(View, wPos);
    float4 pPos = mul(Projection, vPos);
    output.pos = pPos;
    output.uv = input.uv;
    output.worldPos = wPos.xyz; // ワールド座標をPSに渡す
    
    return output;
    */
    output.pos = float4(input.uv * 2.0f - 1.0f, 0.0f, 1.0f);
    output.uv = input.uv;
    output.worldPos = float3(input.uv * 20.0f, 0.0f); // グリッドが見えるよう適当な値を入れる

    return output;
}
