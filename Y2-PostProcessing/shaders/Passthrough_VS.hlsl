
/*
	Vertex shader that simply passes through the (2D) position.
*/

#include "Passthrough_VS.inc"

VS_OUTPUT Passthrough_VS(float3 position : POSITION)
{ 
	VS_OUTPUT output;
	output.screenPos = float4(position.x, position.y, 0.f, 1.f);
	return output;
}
