#include "common.hlsl"

PS_IN main(in VS_IN input)
{
    PS_IN output;

	//positoin=============================
	// ワールド、ビュー、プロジェクション行列を掛け合わせて座標変換を行う
	matrix wvp;
	wvp = mul(World, View);
	wvp = mul(wvp, Projection);
    output.pos = mul(input.pos, wvp);
	
	//color=============================
	
    float4 normal = float4(input.nrm.xyz, 0.0);
    float4 worldNormal = mul(normal, World);
    worldNormal = normalize(worldNormal);
	
    float d = -dot(light.Direction.xyz, worldNormal.xyz);
	
    d = saturate(d);
	
    output.col.xyz = input.col.xyz *d* light.Diffuse.xyz; // 拡散光の影響を乗算
    output.col.xyz += input.col.xyz * light.Ambient.xyz; // アンビエント光を加算
    output.col.xyz += material.Emission.xyz;
    output.col.a = input.col.a * material.Diffuse.a; // アルファ値はそのまま使用
	
	//texture=============================
	// テクスチャ座標はそのまま使用
    output.tex = input.tex;
	
    return output;
}

