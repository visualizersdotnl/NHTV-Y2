
/*
	Year 2 Direct3D 11 workshop template.
	D3D - Core single-thread Direct3D 11 device management and glorified triangle renderer.
*/

#if !defined(D3D_H)
#define D3D_H

namespace D3D 
{
	// Device & context access.
	ID3D11Device *GetDevice();
	ID3D11DeviceContext *GetContext();
}

// Helper classes.
#include "D3D/Buffers.h"
#include "D3D/RenderTarget.h"

namespace D3D
{
	bool Create(ID3D11Device *pDevice, ID3D11DeviceContext *pContext, IDXGISwapChain *pSwapChain, const DXGI_SAMPLE_DESC &multiDesc,
		float renderAspectRatio /* Content */, float displayAspectRatio /* Physical */);
	void Destroy();

	// Frame control.
	void BeginFrame();
	void EndFrame();
	void Flip(unsigned int syncInterval);

	void DrawQuad();
};

#endif // D3D_H
