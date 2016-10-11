
/*
	Year 2 Direct3D 11 workshop template.
	DXGI - Adapter, monitor & display mode management; Direct3D device creation.
*/

#include "Platform.h"
#include "DXGI.h"
#include "Settings.h"

namespace DXGI
{
	// Microsoft has now made a habit out updating their APIs and thus their interface, hence interfaces like IDXGIAdapter1.
	static IDXGIFactory1  *s_pDXGIFactory = nullptr;
	static IDXGIAdapter1  *s_pAdapter     = nullptr;
	static IDXGIOutput    *s_pDisplay     = nullptr;

	// Core Direct3D objects.
	static IDXGISwapChain      *s_pSwapChain  = nullptr;
	static ID3D11Device        *s_pD3D        = nullptr;
	static ID3D11DeviceContext *s_pD3DContext = nullptr;

	// This one is filled in by Create, but might be fiddled with by SetupDialog() later on.
	static DXGI_MODE_DESC s_displayMode;

	// Accessor impl.
	IDXGIFactory1 *GetFactory()           { return s_pDXGIFactory; }
	IDXGISwapChain* GetSwapChain()        { return s_pSwapChain; }
	ID3D11Device *GetDevice()             { return s_pD3D; }
	ID3D11DeviceContext *GetContext()     { return s_pD3DContext; }
	const DXGI_MODE_DESC GetDisplayMode() { return s_displayMode; }

	bool Create(HINSTANCE hInstance, bool windowed)
	{
		if FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void **>(&s_pDXGIFactory)))
		{
			SetLastError("Can not create DXGI 1.1 factory.");
			return false;
		}

		// Get primary display adapter.
		s_pDXGIFactory->EnumAdapters1(0, &s_pAdapter);
		if (nullptr == s_pAdapter)
		{
			// This would be odd, but who knows :-)
			SetLastError("No primary display adapter found.");
			return false;
		}

		// And it's display.
		s_pAdapter->EnumOutputs(0, &s_pDisplay);
		if (nullptr == s_pDisplay)
		{
			SetLastError("No physical display attached to primary display adapter.");
			return false;
		}

		// Get current (desktop) display mode.
		DXGI_MODE_DESC modeToMatch;
		modeToMatch.Width = GetSystemMetrics(SM_CXSCREEN);
		modeToMatch.Height = GetSystemMetrics(SM_CYSCREEN);
		modeToMatch.RefreshRate.Numerator = 0;
		modeToMatch.RefreshRate.Denominator = 0;
		modeToMatch.Format = D3D_BACK_BUFFER_FORMAT;
		modeToMatch.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		modeToMatch.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

		if FAILED(s_pDisplay->FindClosestMatchingMode(&modeToMatch, &s_displayMode, NULL))
		{
			SetLastError("Can not retrieve primary monitor's display mode.");
			return false;
		}

		// Now that we've found a valid back buffer, replace it's format for a gamma-corrected one.
		s_displayMode.Format = D3D_BACK_BUFFER_FORMAT_GAMMA;

		if (true == windowed)
		{
			// Windowed? Pick fixed resolution instead.
			s_displayMode.Width = WINDOWED_RES_X;
			s_displayMode.Height = WINDOWED_RES_Y;
		}

		return true;
	}

	void Destroy()
	{
		SAFE_RELEASE(s_pDisplay);
		SAFE_RELEASE(s_pAdapter);
		SAFE_RELEASE(s_pDXGIFactory);
	}

	void OverrideAdapterAndDisplayMode(unsigned int iAdapter, unsigned int iOutput, const DXGI_MODE_DESC &dispMode)
	{
		// Release primary devices.
		SAFE_RELEASE(s_pDisplay);
		SAFE_RELEASE(s_pAdapter);

		// Get selected devices.
		VERIFY(S_OK == s_pDXGIFactory->EnumAdapters1(iAdapter, &s_pAdapter));
		VERIFY(S_OK == s_pAdapter->EnumOutputs(iOutput, &s_pDisplay));

		// Override display mode.
		s_displayMode = dispMode;
	}

	bool CreateDevice(HWND hWnd, bool windowed, const DXGI_SAMPLE_DESC& multiDesc)
	{
	#if defined(_DEBUG) || defined(_DESIGN)
		const UINT Flags = D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_DEBUG;
//		const UINT Flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#else
		const UINT Flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
	#endif

		// Define supported feature levels.
		// These allow you to make fair assumptions about the display adapter's feature set.
		// See: https://msdn.microsoft.com/en-us/library/windows/desktop/ff476876(v=vs.85).aspx
		const D3D_FEATURE_LEVEL featureLevels[] =
		{
			// This fails on systems that don't explicitly support it (E_INVALIDARG).
//			D3D_FEATURE_LEVEL_11_1,
	
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0
//			D3D_FEATURE_LEVEL_9_3,
//			D3D_FEATURE_LEVEL_9_2,
//			D3D_FEATURE_LEVEL_9_1
		};

		// FIXME: decide what we'll do with this information.
		D3D_FEATURE_LEVEL featureLevel;

		// Define swap chain and associate it with render window.
		DXGI_SWAP_CHAIN_DESC swapDesc;
		memset(&swapDesc, 0, sizeof(swapDesc));
		swapDesc.BufferDesc = s_displayMode;
		swapDesc.SampleDesc = multiDesc;
		swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapDesc.BufferCount = 2;
		swapDesc.OutputWindow = hWnd;
		swapDesc.Windowed = windowed;
		swapDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapDesc.Flags = 0; // DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		// Create swap chain, device & single-threaded context.
		HRESULT hRes = D3D11CreateDeviceAndSwapChain(
			s_pAdapter,
			D3D_DRIVER_TYPE_UNKNOWN, // A must if s_pAdapter isn't NULL; documented under the remarks section of this call.
			NULL,
			Flags,
			featureLevels, ARRAYSIZE(featureLevels),
			D3D11_SDK_VERSION,
			&swapDesc,
			&s_pSwapChain,
			&s_pD3D,
			&featureLevel,
			&s_pD3DContext);
		if (S_OK == hRes)
		{
			// Block ALT+ENTER et cetera.
			hRes = s_pDXGIFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_WINDOW_CHANGES);
			ASSERT(hRes == S_OK);
			
			return true;
		}
		else
		{
			// Failed: build a somewhat meaningful message.
			std::stringstream message;
			message << "Can't create Direct3D 11.0 device.\n\n";
			message << ((true == windowed) ? "Type: windowed.\n" : "Type: full screen.\n");
			message << "Resolution: " << s_displayMode.Width << "*" << s_displayMode.Height << ".\n";
			if (0 != multiDesc.Quality) message << "Multi-sampling enabled (" << multiDesc.Count << " taps).\n";

			// Set it, and bail.
			SetLastError(message.str());
			return false;
		}

		return true;
	}

	void DestroyDevice(bool windowed)
	{
		if (nullptr != s_pSwapChain && false == windowed)
		{
			// Take the swap chain out of full screen mode.
			s_pSwapChain->SetFullscreenState(FALSE, nullptr);
		}

		SAFE_RELEASE(s_pD3DContext);
		SAFE_RELEASE(s_pD3D);
		SAFE_RELEASE(s_pSwapChain);
	}
};
