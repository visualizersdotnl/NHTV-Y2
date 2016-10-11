
/*
	Pixel shader that just spits out a fixed color.
*/

#include "Passthrough_VS.inc"

/*
struct VS_OUTPUT
{
	float4 screenPos : SV_Position;
	float4 color : COLOR;
	float2 UV : TEXCOORD0;
};

Texture2D sprite : register(t0);
SamplerState spriteSampler : register(s0);
*/

float4 Passthrough_PS(in VS_OUTPUT input) : SV_Target0
{
	return float4(1.f, 1.f, 1.f, 1.f);
//	return input.color*sprite.Sample(spriteSampler, input.UV);
}
