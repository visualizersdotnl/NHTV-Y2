
/*
	Year 2 Direct3D 11 workshop template.
	D3D - Core single-thread Direct3D 11 device management and glorified triangle renderer.

	A few things are checked by ASSERT() instead of an error check, as they simply should not fail.
	Otherwise it's food for the generic exception handler in Win32.cpp!
	
	To do:
	- This could live with a few more assertions and less (static) globals.
	- Fix inadvertent Flip() calls.
*/

#include "Platform.h"
#include "D3D.h"
#include "Settings.h"

// Sub-classes.
#include "D3D/RenderTarget.h"

// This is some code that nicely encapsulates different kind of Direct3D buffers using a template.
// It's not used yet.
// #include "D3D/Buffers.h"

// Include headers with the passtrough shader's bytecode.
namespace VS 
{ 
	#include "../shaders/Passthrough_VS.h" 
}

namespace PS 
{ 
	#include "../shaders/Passthrough_PS.h" 
}

namespace D3D
{
	// Local copies.
	static ID3D11Device        *s_pDev       = nullptr;
	static ID3D11DeviceContext *s_pContext   = nullptr;
	static IDXGISwapChain      *s_pSwapChain = nullptr;

	// Resources.
	// This won't scale well at all, so I'd advise wrapping them in renderer-specific objects.
	static RenderTarget *s_pBackBuffer = nullptr;
	static ID3D11RasterizerState *s_pRasterizerState = nullptr;
	static ID3D11BlendState *s_pBlendState = nullptr;
	static ID3D11SamplerState *s_pSamplerState = nullptr;
	static ID3D11Buffer *s_pQuadVB = nullptr;
	static ID3D11InputLayout *s_pInputLayout = nullptr;
	static ID3D11VertexShader *s_pVertexShader = nullptr;
	static ID3D11PixelShader *s_pPixelShader = nullptr;

	// Viewports:
	D3D11_VIEWPORT s_backVP;    // Full-size back buffer viewport.
	D3D11_VIEWPORT s_backAdjVP; // Viewport that centers our scene on the back buffer (letterboxing).
	D3D11_VIEWPORT s_sceneVP;   // Aspect-ratio adjusted scene viewport (for custom render targets).

	// Vertices (6) for a full screen quad (2 triangles).
	const Vector3 kQuadVertices[] =
	{
		Vector3(-1.f,  1.f, 0.f), // 0
		Vector3( 1.f,  1.f, 0.f), // 1
		Vector3( 1.f, -1.f, 0.f), // 3
		Vector3(-1.f,  1.f, 0.f), // 0
		Vector3( 1.f, -1.f, 0.f), // 3
		Vector3(-1.f, -1.f, 0.f), // 2						
	};

	const D3D11_INPUT_ELEMENT_DESC kElements[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	// Accessors.
	ID3D11Device *GetDevice() { ASSERT(nullptr != s_pDev); return s_pDev; }
	ID3D11DeviceContext *GetContext() { ASSERT(nullptr != s_pContext); return s_pContext; }

	bool Create(ID3D11Device *pDevice, ID3D11DeviceContext *pContext, IDXGISwapChain *pSwapChain, const DXGI_SAMPLE_DESC &multiDesc,
		float renderAspectRatio /* Content */, float displayAspectRatio /* Physical */)
	{
		// If I'd pass this in an object containers these could be references, since I'm assuming they are all valid at this point.
		// For now I'll resort to assertions.
		ASSERT(nullptr != pDevice);
		ASSERT(nullptr != pContext);
		ASSERT(nullptr != pSwapChain);

		// Assign to locals.
		s_pDev = pDevice;
		s_pContext = pContext;
		s_pSwapChain = pSwapChain;

		// Create render target for back buffer.
		{
			// Fetch back buffer.
			ComPtr<ID3D11Texture2D> texture;
			HRESULT hRes = s_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void **>(texture.GetAddressOf()));
			ASSERT(S_OK == hRes);

			// Create target view.
			ComPtr<ID3D11RenderTargetView> targetView;
			hRes = s_pDev->CreateRenderTargetView(texture.Get(), nullptr, &targetView);
			ASSERT(S_OK == hRes);

			// And bundle it up in a nice little container.
			s_pBackBuffer = new RenderTarget(D3D_BACK_BUFFER_FORMAT_GAMMA, texture.Detach(), targetView.Detach(), nullptr);
		}

		// Bind back buffer.
		ID3D11RenderTargetView *pView = s_pBackBuffer->GetTargetView();
		s_pContext->OMSetRenderTargets(1, &pView, nullptr);

		// We'll be rendering triangle lists.
		s_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Create and set a fixed rasterizer state.
		D3D11_RASTERIZER_DESC rasterDesc;
		memset(&rasterDesc, 0, sizeof(rasterDesc));
		rasterDesc.FillMode = D3D11_FILL_SOLID;
		rasterDesc.CullMode = D3D11_CULL_BACK;
		rasterDesc.FrontCounterClockwise = FALSE;
		rasterDesc.DepthBias = 0;
		rasterDesc.DepthBiasClamp = 0;
		rasterDesc.SlopeScaledDepthBias = 0;
		rasterDesc.DepthClipEnable = TRUE;
		rasterDesc.ScissorEnable = FALSE;
		rasterDesc.MultisampleEnable = TRUE;
		rasterDesc.AntialiasedLineEnable = FALSE;
		VERIFY(S_OK == s_pDev->CreateRasterizerState(&rasterDesc, &s_pRasterizerState));

		s_pContext->RSSetState(s_pRasterizerState);

		// Fetch back buffer dimensions.
		D3D11_TEXTURE2D_DESC backbufferDesc;
		s_pBackBuffer->GetTexture()->GetDesc(&backbufferDesc);
		const float viewWidth = (float) backbufferDesc.Width;
		const float viewHeight = (float) backbufferDesc.Height;

		// Define full viewport (covering the entire back buffer).
		s_backVP.TopLeftX = 0.f;
		s_backVP.TopLeftY = 0.f;
		s_backVP.Width = viewWidth;
		s_backVP.Height = viewHeight;
		s_backVP.MinDepth = 0.f;
		s_backVP.MaxDepth = 1.f;

		// Calculate viewports (full & aspect ratio adjusted).
		float xResAdj, yResAdj;
		if (displayAspectRatio < renderAspectRatio)
		{
			// Bars on top and bottom.
			const float scale = displayAspectRatio / renderAspectRatio;
			xResAdj = s_backVP.Width;
			yResAdj = s_backVP.Height*scale;
		}
		else if (displayAspectRatio > renderAspectRatio)
		{
			// Bars left and right.
			const float scale = renderAspectRatio / displayAspectRatio;
			xResAdj = s_backVP.Width*scale;
			yResAdj = s_backVP.Height;
		}
		else // No adjustment necessary (ideal).
		{
			xResAdj = s_backVP.Width;
			yResAdj = s_backVP.Height;
		}

		// FIXME: bars on both sides?

		// Back buffer viewport adjusted to fit scene.
		s_backAdjVP.Width = xResAdj;
		s_backAdjVP.Height = yResAdj;
		s_backAdjVP.TopLeftX = (s_backVP.Width - xResAdj) / 2.f;
		s_backAdjVP.TopLeftY = (s_backVP.Height - yResAdj) / 2.f;
		s_backAdjVP.MinDepth = 0.f;
		s_backAdjVP.MaxDepth = 1.f;

		// Scene viewport (for custom render targets).
		s_sceneVP.Width = xResAdj;
		s_sceneVP.Height = yResAdj;
		s_sceneVP.TopLeftX = 0.f;
		s_sceneVP.TopLeftY = 0.f;
		s_sceneVP.MinDepth = 0.f;
		s_sceneVP.MaxDepth = 1.f;

		// Set full viewport by default.
		s_pContext->RSSetViewports(1, &s_backVP);

		// Default (opaque) blend state (NULL is valid in this case).
		s_pBlendState = nullptr;

		// Create fixed sampler state (tri-linear, wrap).
		D3D11_SAMPLER_DESC samplerDesc;
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.MipLODBias = 0.f;
		samplerDesc.MaxAnisotropy = 4;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		samplerDesc.BorderColor[0] = 0.f;
		samplerDesc.BorderColor[1] = 0.f;
		samplerDesc.BorderColor[2] = 0.f;
		samplerDesc.BorderColor[3] = 0.f;
		samplerDesc.MinLOD = 0.f;
		samplerDesc.MaxLOD = FLT_MAX;
		VERIFY(S_OK == s_pDev->CreateSamplerState(&samplerDesc, &s_pSamplerState));

		// Create small vertex buffer for 2 triangles.
		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT; // D3D11_USAGE_IMMUTABLE;
		bufferDesc.ByteWidth = (UINT) 6*sizeof(Vector3);
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA bufferData;
		bufferData.pSysMem = kQuadVertices;
		bufferData.SysMemPitch = 0;
		bufferData.SysMemSlicePitch = 0;

//		ID3D11Buffer *s_pQuadVB = nullptr;
		VERIFY(S_OK == s_pDev->CreateBuffer(&bufferDesc, &bufferData, &s_pQuadVB));

		// Create an input layout for the vertex buffer (describing what exactly this buffer contains).
		const D3D11_INPUT_ELEMENT_DESC elemDesc[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		// This is verified against the vertex shader's signature.
		HRESULT hResult = s_pDev->CreateInputLayout(elemDesc, 1, VS::g_main, sizeof(VS::g_main), &s_pInputLayout);
		ASSERT(S_OK == hResult);

		// Create vertex shader (their global names should be fixed, FIXME!).
		VERIFY(S_OK == s_pDev->CreateVertexShader(VS::g_main, sizeof(VS::g_main), nullptr, &s_pVertexShader));
		VERIFY(S_OK == s_pDev->CreatePixelShader(PS::g_main, sizeof(PS::g_main), nullptr, &s_pPixelShader));

		// Exercise to the reader: 
		// - Create and use a 2D texture.
		// - Create and set a depth (stencil) buffer?

		return true;
	}

	void Destroy()
	{
		SAFE_RELEASE(s_pPixelShader);
		SAFE_RELEASE(s_pVertexShader);
		SAFE_RELEASE(s_pInputLayout);
		SAFE_RELEASE(s_pQuadVB);
		SAFE_RELEASE(s_pSamplerState);
		SAFE_RELEASE(s_pBlendState);
		SAFE_RELEASE(s_pRasterizerState);
		delete s_pBackBuffer;
	}

	// Draw our frame (in this case, stupid triangle).
	void RenderFrame()
	{
		// Set full viewport.
		s_pContext->RSSetViewports(1, &s_backVP);

		// Clear it (black).
		// This is something you can do in different ways that beat clearing the entire buffer each frame.
		const float black[4] = { 0.f, 0.f, 0.4f, 0.f };
		s_pContext->ClearRenderTargetView(s_pBackBuffer->GetTargetView(), black);

		// Set adjusted viewport.
		s_pContext->RSSetViewports(1, &s_backAdjVP);

		// Bind quad vertex buffer.
		const UINT stride = sizeof(Vector3); // Size of each element (a single 3D point).
		const UINT offset = 0;
		s_pContext->IASetVertexBuffers(0, 1, &s_pQuadVB, &stride, &offset);

		// And it's input layout, so the shader can be fed.
		s_pContext->IASetInputLayout(s_pInputLayout);

		// Set vertex & pixel shader.
		s_pContext->VSSetShader(s_pVertexShader, nullptr, 0);
		s_pContext->PSSetShader(s_pPixelShader, nullptr, 0);

		// Draw it, without using an index buffer.
		s_pContext->Draw(6, 0);

		// Restore full viewport.
		s_pContext->RSSetViewports(1, &s_backVP);
	}

	// Call only if a frame has been drawn, and, the output window has focus.
	void Flip(unsigned int syncInterval)
	{
		const HRESULT hRes = s_pSwapChain->Present(syncInterval, 0);
//		ASSERT(S_OK == hRes); // FIXME!
	}
}
