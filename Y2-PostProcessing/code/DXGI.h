
/*
	Year 2 Direct3D 11 workshop template.
	DXGI - Adapter, monitor & display mode management; Direct3D device creation.

	A namespace is used here in favour of a class; we need a single instance and to me
	the Singleton pattern feels like a useless design pattern that only allows room for error
	by further obfuscating the intended use, or worse, confusing the order
	in which critical sytems are initialized.

	The same pattern is followed for the core D3D module (D3D.cpp/D3D.h).

	This does force you to explicitly keep track of and think about when and where
	you call certain functions, but that's a realistic scenario.
*/

#if !defined(DXGI_H)
#define DXGI_H

namespace DXGI
{
	// Handles identifying the primary display & adapter and enumartion of all available options.
	bool Create(HINSTANCE hInstance, bool windowed);
	void Destroy();

	// Ability to override the primary adapter, display & mode (as used by the setup dialog).
	void OverrideAdapterAndDisplayMode(unsigned int iAdapter, unsigned int iOutput, const DXGI_MODE_DESC &dispMode);

	// Handles creation & destruction of a swap chain, (single-threaded) Direct3D 11 device & context.
	bool CreateDevice(HWND hWnd, bool windowed, const DXGI_SAMPLE_DESC &multiDesc);
	void DestroyDevice(bool windowed);

	// Accessors.
	IDXGIFactory1       *GetFactory();
	IDXGISwapChain      *GetSwapChain();
	ID3D11Device        *GetDevice();
	ID3D11DeviceContext *GetContext();

	const DXGI_MODE_DESC GetDisplayMode();
};

#endif // DXGI_H
