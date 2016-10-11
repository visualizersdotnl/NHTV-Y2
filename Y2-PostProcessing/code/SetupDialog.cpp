
/*
	Year 2 Direct3D 11 workshop template.
	SetupDialog - App. setup dialog.

	This won't win any beauty contest but it's functional and give how terrible the Visual Studio resource
	editor is, I'm fine with it.

	Unicode is assumed!
*/

#include "Platform.h"
#include <iomanip> // For std::setprecision().
#include "Settings.h"
#include "../VS/Resources/resource.h"

// Aspect ratio representation (with some elementary school math for reduction).
class AspectRatio
{
public:
	AspectRatio(unsigned int width, unsigned int height) :
		width(width), height(height)
	{
		ASSERT(0 != width && 0 != height);
		Reduce();
	}

private:
	unsigned int GCD(unsigned int A, unsigned int B) const
	{
		return (0 == B) ? A : GCD(B, A%B);
	}

	void Reduce()
	{
		const unsigned int divisor = GCD(width, height);
		numerator = width / divisor;
		denominator = height / divisor;
	}

public:
	float GetRatio() const
	{
		return (float)numerator / denominator;
	}

	const std::string GetDesc() const
	{
		std::stringstream desc;
		desc << numerator << ":" << denominator;

		// A wild guess to determine if this reduction might look unintuitive and thus could use extra information.
		if (numerator > 16 || denominator > 10)
		{
			// This happens (e.g. 1600x1024 reduces to 25:16), so add a bit more information.
			desc << " (" << width << "x" << height << ", ratio: " << std::setprecision(3) << GetRatio() << ")";
		}

		return desc.str();
	}

	bool operator ==(const AspectRatio &RHS) const
	{
		return numerator == RHS.numerator && denominator == RHS.denominator;
	}

	bool operator <(const AspectRatio &RHS) const
	{
		return numerator < RHS.numerator;
	}

private:
	const unsigned int width, height;
	unsigned int numerator, denominator;
};

// Pointers to settings variables passed to SetupDialog().
// Again, yes, this should be a POD structure.
static unsigned int   *s_iAdapter;
static unsigned int   *s_iOutput;
static DXGI_MODE_DESC *s_mode;
static float          *s_aspectRatio;
static unsigned int   *s_multiSamples;
static bool           *s_windowed;
static bool           *s_vSync;

// Temporary (saves me the pain of passing it around all the time), set by SetupDialog().
static IDXGIFactory1 *s_pDXGIFactory;

static std::vector<DEVMODEW>       s_curOutputModes;
static std::vector<DXGI_MODE_DESC> s_enumeratedModes;
static std::list<AspectRatio>      s_aspectRatios;

static bool UpdateOutputs(HWND hDialog, unsigned int iAdapter)
{
	// Wipe combobox and mode array.
	SendDlgItemMessage(hDialog, IDC_COMBO_OUTPUT, CB_RESETCONTENT, 0, 0);
	s_curOutputModes.clear();

	IDXGIAdapter1 *pAdapter = nullptr;
	VERIFY(S_OK == s_pDXGIFactory->EnumAdapters1(iAdapter, &pAdapter));

	UINT iOutput = 0;
	IDXGIOutput *pOutput = nullptr;
	while (DXGI_ERROR_NOT_FOUND != pAdapter->EnumOutputs(iOutput++, &pOutput))
	{
		DXGI_OUTPUT_DESC desc;
		VERIFY(S_OK == pOutput->GetDesc(&desc));

		MONITORINFOEXW monInfoEx;
		DISPLAY_DEVICEW dispDev;
		monInfoEx.cbSize = sizeof(MONITORINFOEXW);
		dispDev.cb = sizeof(DISPLAY_DEVICEW);

		if (FALSE == GetMonitorInfoW(desc.Monitor, &monInfoEx) || FALSE == EnumDisplayDevicesW(monInfoEx.szDevice, 0, &dispDev, 0))
		{
			// This shouldn't happen, right?
			ASSERT(0);
			wcscpy_s(dispDev.DeviceString, 128, L"Unidentified output device");
		}

		// What shows up here would more often than not be "Plug & Play Monitor" or something likewise.
		// But if someone actually takes the time to install a monitor driver that's what you'll get.
		std::wstringstream monitorName;
		monitorName << "#" << iOutput << ": " << dispDev.DeviceString;
		SendDlgItemMessageW(hDialog, IDC_COMBO_OUTPUT, CB_ADDSTRING, 0, (LPARAM)monitorName.str().c_str());

		// Attempt to grab active display mode to feed DXGI's FindClosestMatchingMode().
		DEVMODEW curMode;
		curMode.dmSize = sizeof(DEVMODEW);
		curMode.dmDriverExtra = 0;
		if (FALSE == EnumDisplaySettingsW(monInfoEx.szDevice, ENUM_CURRENT_SETTINGS, &curMode))
			memset(&curMode, 0, sizeof(DEVMODEW)); // Invalidate. Will feed zeroes resulting in a good old mismatch.

		s_curOutputModes.push_back(curMode);

		pOutput->Release();
	}

	SAFE_RELEASE(pAdapter);

	if (0 == iOutput)
	{
		// No outputs detected: a valid scenario, for example a secondary graphics adapter without any monitor attached.
		// Add this string and return false so a few controls can be greyed out.
		SendDlgItemMessageW(hDialog, IDC_COMBO_OUTPUT, CB_ADDSTRING, 0, (LPARAM)L"No display attached to adapter (attach and/or re-select).");
	}

	// Just select the first one.
	SendDlgItemMessage(hDialog, IDC_COMBO_OUTPUT, CB_SETCURSEL, 0, 0);

	return 0 != iOutput;
}

static void UpdateDisplayModes(HWND hDialog, unsigned int iAdapter, unsigned int iOutput)
{
	// Wipe comboboxes (modes & aspect ratios).
	SendDlgItemMessage(hDialog, IDC_COMBO_RESOLUTION, CB_RESETCONTENT, 0, 0);
	SendDlgItemMessage(hDialog, IDC_COMBO_ASPECT, CB_RESETCONTENT, 0, 0);

	// Add (recommended) automatic aspect ratio adjustment setting, and select it.
	// In 99% of the cases this is what's correct.
	SendDlgItemMessage(hDialog, IDC_COMBO_ASPECT, CB_ADDSTRING, 0, (LPARAM)L"Automatic (recommended)");
	SendDlgItemMessage(hDialog, IDC_COMBO_ASPECT, CB_SETCURSEL, 0, 0);

	// Get adapter & display objects.
	IDXGIAdapter1 *pAdapter = nullptr;
	IDXGIOutput *pOutput = nullptr;
	VERIFY(S_OK == s_pDXGIFactory->EnumAdapters1(iAdapter, &pAdapter));
	VERIFY(S_OK == pAdapter->EnumOutputs(iOutput, &pOutput));

	// FIXME!
	if (nullptr == pOutput)
	{
	}

	// Get list of available display modes.
	UINT numModes;
	VERIFY(S_OK == pOutput->GetDisplayModeList(D3D_BACK_BUFFER_FORMAT, 0, &numModes, nullptr));
	s_enumeratedModes.resize(numModes);
	VERIFY(S_OK == pOutput->GetDisplayModeList(D3D_BACK_BUFFER_FORMAT, 0, &numModes, &s_enumeratedModes[0]));

	// Clear aspect ratio list.
	s_aspectRatios.clear();

	// Populate aspect ratio list.
	for (auto &mode : s_enumeratedModes)
	{
		// Create a (hopefully) descriptive string.
		std::wstringstream resolution;
		resolution << mode.Width << "x" << mode.Height << " @ " << mode.RefreshRate.Numerator / mode.RefreshRate.Denominator << "Hz";
		SendDlgItemMessage(hDialog, IDC_COMBO_RESOLUTION, CB_ADDSTRING, 0, (LPARAM)resolution.str().c_str());

		s_aspectRatios.push_back(AspectRatio(mode.Width, mode.Height));
	}

	// Sort aspect ratios and remove duplicates (mostly due to equal resolutions at different refresh rates).
	s_aspectRatios.sort();
	s_aspectRatios.unique();

	// Add derived aspect ratios (retain automatic setting as default) to combobox.
	for (auto &ratio : s_aspectRatios)
	{
		SendDlgItemMessage(hDialog, IDC_COMBO_ASPECT, CB_ADDSTRING, 0, (LPARAM)ToUnicode(ratio.GetDesc()).c_str());
	}

	// Select first display mode by default; now we'll try to find the active mode.
	SendDlgItemMessage(hDialog, IDC_COMBO_RESOLUTION, CB_SETCURSEL, 0, 0);

	// Find active (typically desktop) display mode, if there is one.
	// This has been a pain to fix.
	DXGI_MODE_DESC modeToMatch;
	modeToMatch.Width = s_curOutputModes[iOutput].dmPelsWidth;
	modeToMatch.Height = s_curOutputModes[iOutput].dmPelsHeight;
	modeToMatch.RefreshRate.Numerator = 0; // At least it seems to be able to get this right.
	modeToMatch.RefreshRate.Denominator = 0;
	modeToMatch.Format = D3D_BACK_BUFFER_FORMAT;
	modeToMatch.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	modeToMatch.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Try to find the closest match.
	DXGI_MODE_DESC curMode;
	if (S_OK == pOutput->FindClosestMatchingMode(&modeToMatch, &curMode, NULL))
	{
		for (size_t iMode = 0; iMode < s_enumeratedModes.size(); ++iMode)
		{
			const DXGI_MODE_DESC &mode = s_enumeratedModes[iMode];
			if (0 == memcmp(&mode, &curMode, sizeof(DXGI_MODE_DESC)))
			{
				// Exact match found. Select it.
				SendDlgItemMessage(hDialog, IDC_COMBO_RESOLUTION, CB_SETCURSEL, (WPARAM)iMode, 0);
				break;
			}
		}
	}

	pOutput->Release();
	pAdapter->Release();
}

static INT_PTR CALLBACK DialogProc(HWND hDialog, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	// This is our cue to populate (initialize) the dialog box.
	case WM_INITDIALOG:
	{
		// Enumerate display adapters.
		UINT iAdapter = 0;
		IDXGIAdapter1 *pAdapter = nullptr;
		while (DXGI_ERROR_NOT_FOUND != s_pDXGIFactory->EnumAdapters1(iAdapter++, &pAdapter))
		{
			DXGI_ADAPTER_DESC1 desc;
			VERIFY(S_OK == pAdapter->GetDesc1(&desc));
			SendDlgItemMessageW(hDialog, IDC_COMBO_DISPLAY_ADAPTER, CB_ADDSTRING, 0, (LPARAM)desc.Description);
			pAdapter->Release();
		}

		// Index 0 is always the default (primary) display adapter: select it.		
		SendDlgItemMessage(hDialog, IDC_COMBO_DISPLAY_ADAPTER, CB_SETCURSEL, 0, 0);

		// Select primary adapter and display (verified to be in working order by DXGI.cpp).
		if (true == UpdateOutputs(hDialog, 0))
			UpdateDisplayModes(hDialog, 0, 0);

		// Add minimal multi-sampling options and turn it off by default.
		SendDlgItemMessage(hDialog, IDC_COMBO_MULTI, CB_ADDSTRING, 0, (LPARAM)L"Off");
		SendDlgItemMessage(hDialog, IDC_COMBO_MULTI, CB_ADDSTRING, 0, (LPARAM)L"2x");
		SendDlgItemMessage(hDialog, IDC_COMBO_MULTI, CB_ADDSTRING, 0, (LPARAM)L"4x");
		SendDlgItemMessage(hDialog, IDC_COMBO_MULTI, CB_ADDSTRING, 0, (LPARAM)L"8x");
		SendDlgItemMessage(hDialog, IDC_COMBO_MULTI, CB_SETCURSEL, 0, 0);
	}

	return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		// OK button was pressed, let's gather all the selected settings.
		case IDOK:
		{
			// - Toggles
			*s_windowed = BST_CHECKED == IsDlgButtonChecked(hDialog, IDC_CHECK_WINDOWED);
			*s_vSync = BST_CHECKED == IsDlgButtonChecked(hDialog, IDC_CHECK_VSYNC);

			if (false == *s_windowed)
			{
				// Adapter & output (or display if you will).
				*s_iAdapter = (unsigned int)SendDlgItemMessage(hDialog, IDC_COMBO_DISPLAY_ADAPTER, CB_GETCURSEL, 0, 0);
				*s_iOutput = (unsigned int)SendDlgItemMessage(hDialog, IDC_COMBO_OUTPUT, CB_GETCURSEL, 0, 0);

				// Display mode.
				const size_t iMode = (size_t)SendDlgItemMessage(hDialog, IDC_COMBO_RESOLUTION, CB_GETCURSEL, 0, 0);
				*s_mode = s_enumeratedModes[iMode];

				// Aspect ratio.
				const size_t iAspect = (size_t)SendDlgItemMessage(hDialog, IDC_COMBO_ASPECT, CB_GETCURSEL, 0, 0);
				if (0 == iAspect)
				{
					// Automatic mode.
					*s_aspectRatio = -1.f;
				}
				else
				{
					// Get override from list (a feature recommended by KB/Farbrausch).
					auto iListRatio = s_aspectRatios.begin();
					std::advance(iListRatio, iAspect - 1);
					ASSERT(iListRatio != s_aspectRatios.end());
					*s_aspectRatio = iListRatio->GetRatio();
				}

				// Force format to gamma-corrected one.
				s_mode->Format = D3D_BACK_BUFFER_FORMAT_GAMMA;
			}
			else
			{
				// Use primary adapter and display (verified to be in working order by DXGI.cpp).
				*s_iAdapter = 0;
				*s_iOutput = 0;

				// Aspect ratio will be adapted (square pixels).
				*s_aspectRatio = -1.f;

				// Display mode was defined in DXGI.cpp for windowed mode.
			}

			// Multi-sampling.
			const unsigned int iMultiSamples = (unsigned int)SendDlgItemMessage(hDialog, IDC_COMBO_MULTI, CB_GETCURSEL, 0, 0);
			switch (iMultiSamples) // FIXME: not pretty, but it's hardcoded anyway.
			{
			case 0:
				*s_multiSamples = 1;
				break;

			case 1:
				*s_multiSamples = 2;
				break;

			case 2:
				*s_multiSamples = 4;
				break;

			case 3:
				*s_multiSamples = 8;
				break;

			default:
				ASSERT(0);
				*s_multiSamples = 1;
			}
		}

		// Cancel (and likely exit).
		case IDCANCEL:
			EndDialog(hDialog, LOWORD(wParam));
			return 0;

		// Refresh outputs and display modes.
		case IDC_COMBO_DISPLAY_ADAPTER:
			switch (HIWORD(wParam))
			{
			case CBN_SELCHANGE:
				// Adapter changed: update output plus mode & aspect ratio list (for output #0).
				*s_iAdapter = (unsigned int)SendDlgItemMessage(hDialog, IDC_COMBO_DISPLAY_ADAPTER, CB_GETCURSEL, 0, 0);

				// If an adapter has no outputs, prohibit further configuration.
				const bool hasOutputs = UpdateOutputs(hDialog, *s_iAdapter);
				EnableWindow(GetDlgItem(hDialog, IDC_COMBO_OUTPUT), hasOutputs);
				EnableWindow(GetDlgItem(hDialog, IDC_COMBO_RESOLUTION), hasOutputs);
				EnableWindow(GetDlgItem(hDialog, IDC_COMBO_ASPECT), hasOutputs);
				EnableWindow(GetDlgItem(hDialog, IDOK), hasOutputs);

				// Windowed mode seems impossible as well in case no outputs are attached.
				EnableWindow(GetDlgItem(hDialog, IDC_CHECK_WINDOWED), hasOutputs);

				if (true == hasOutputs)
				{
					// But we *do* have an output, so populate the display modes drop-down.
					UpdateDisplayModes(hDialog, *s_iAdapter, 0);
				}

				return 0;
			}

			break;

		// Output (display) changed: refresh display modes.
		case IDC_COMBO_OUTPUT:
			switch (HIWORD(wParam))
			{
			case CBN_SELCHANGE:
				*s_iAdapter = (unsigned int)SendDlgItemMessage(hDialog, IDC_COMBO_DISPLAY_ADAPTER, CB_GETCURSEL, 0, 0);
				*s_iOutput = (unsigned int)SendDlgItemMessage(hDialog, IDC_COMBO_OUTPUT, CB_GETCURSEL, 0, 0);

				// Output changed: update mode & aspect ratio list.
				UpdateDisplayModes(hDialog, *s_iAdapter, *s_iOutput);

				return 0;
			}

			break;

		case IDC_CHECK_WINDOWED:
			switch (HIWORD(wParam))
			{
			case BN_CLICKED:
				// Grey out specific settings if windowed box is checked.
				*s_windowed = BST_CHECKED == IsDlgButtonChecked(hDialog, IDC_CHECK_WINDOWED);
				const BOOL enable = false == *s_windowed;
				EnableWindow(GetDlgItem(hDialog, IDC_COMBO_DISPLAY_ADAPTER), enable);
				EnableWindow(GetDlgItem(hDialog, IDC_COMBO_OUTPUT), enable);
				EnableWindow(GetDlgItem(hDialog, IDC_COMBO_RESOLUTION), enable);
				EnableWindow(GetDlgItem(hDialog, IDC_COMBO_ASPECT), enable);
				EnableWindow(GetDlgItem(hDialog, IDC_CHECK_VSYNC), enable);

				return 0;
			}

			break;
		}
	}

	return FALSE;
}

bool SetupDialog(
	HINSTANCE hInstance,
	unsigned int &iAdapter,
	unsigned int &iOutput,
	DXGI_MODE_DESC &mode,
	float &aspectRatio,
	unsigned int &multiSamples,
	bool &windowed,
	bool &vSync,
	IDXGIFactory1 *pDXGIFactory)
{
	// Ugly: assign references to local pointers.
	s_iAdapter = &iAdapter;
	s_iOutput = &iOutput;
	s_mode = &mode;
	s_aspectRatio = &aspectRatio;
	s_multiSamples = &multiSamples;
	s_windowed = &windowed;
	s_vSync = &vSync;
	s_pDXGIFactory = pDXGIFactory;

	switch (DialogBox(hInstance, MAKEINTRESOURCE(IDD_SETUP), 0, DialogProc))
	{
	case IDOK:
		return true;

	case IDCANCEL:
		return false;

	default:
		// I'm setting an error message here, but if we ever land up in this spot something is terribly off.
		SetLastError("Can't spawn setup dialog.");
		return false;
	}
}
