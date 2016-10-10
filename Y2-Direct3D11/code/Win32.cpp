
/*
	Year 2 Direct3D 11 workshop template.
	by Niels J. de Wit (wit.n@ade-nhtv.nl)

	Adapted from a tried & tested demoscene engine.

	This influences a few simple design decisions that may or may not seem a little simplistic or strange,
	or even outdated, conflicting notation practices, but that's the product of an evolutionary codebase 
	and something you'll definitely encounter during your professional career as a programmer.

	Also, sorry for the lack of flashy C++. Some of this code was initially written over 4 years ago and I'm 
	really not the "just cause" kind of guy when it comes to using language features.

	However I *did* take out (most) filth and reviewed/added comments.
	Generally I wouldn't comment like this (code should be self-explanatory, meaning that whenever
	you're about to place a comment a rule of thumb can be to first ask yourself why the code is
	not self-explanatory).

	The compiler settings do not disable C++ exceptions (though I do not rely on them) thus negating
	any code generation gain; the 'Design' build is a copy of the debug build with some optimization enabled,
	so as to provide a faster executable with full debug information.

	There's a math library of mine (wrote it last summer I think) involved that's largely untested,
	so have a blast with it if you want and please come talk to me if I screwed up somewhere or if you'd
	like to see new features.

	Win32 - Entry point, frame loop & window management.

	To-do (low priority):
	- Fix the cock up in the setup dialog (adapters without an output, what to do?).
	- Create easier challenges for a repeat Direct3D 11 workshop.
	- Re-check that ResizeBuffers() mentioned.
	- Aspect ratio doesn't account for cases were bars are needed on all sides.
	- FIXMEs.

	Known issues:
	- Probably a few :-)
*/

// For small to mid-size projects I make it a habit to include a header that includes
// the OS, API, STL & some local essentials. I dislike precompiled headers, but obviously
// this strategy does not scale very well.
#include "Platform.h"

// For SIMD support check.
#include <intrin.h> 
#include "platform/CPUID.h"

#include "../VS/Resources/resource.h"
#include "Settings.h"
#include "DXGI.h"
#include "SetupDialog.h"
#include "D3D.h"

// Configuration: windowed or full screen.
bool s_windowed = WINDOWED_DEV; // Can be modified later by setup dialog.

// Global error message.
// This is a simple mechanism to relay an error message in applications that do not need to recover from failure.
static std::string s_lastError;
void SetLastError(const std::string &message) { s_lastError = message; }

// Render/application window variables.
static bool s_classRegged = false;
static HWND s_hWnd = NULL;
static bool s_wndIsActive; // Set by WindowProc()

// Window message loop.
static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_LBUTTONDOWN:
		// Drag.
		break;

	case WM_LBUTTONUP:
		// Drop.
		break;

	case WM_MOUSEMOVE:
		// Move.
		break;

	case WM_CLOSE:
		PostQuitMessage(0); // Terminate message loop.
		s_hWnd = NULL;      // DefWindowProc() will call DestroyWindow().
		break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_ESCAPE:
			PostMessage(hWnd, WM_CLOSE, 0, 0);
			break;

		case VK_SPACE:
			// Debug (un)pause.
			break;
		}

		if (0)
		{
			// Debug move.
			if (wParam == 'A') {}
			else if (wParam == 'D') {}
			else if (wParam == 'W') {}
			else if (wParam == 'S') {}
			else if (wParam == 'Q') {}
			else if (wParam == 'E') {}
			else if (wParam == VK_RETURN) {}
		}

		break;

	case WM_ACTIVATE:
		switch (LOWORD(wParam))
		{
		case WA_ACTIVE:
		case WA_CLICKACTIVE:
			if (false == s_windowed)
			{
				// (Re-)assign WS_EX_TOPMOST style.
				SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			}

			s_wndIsActive = true;
			break;

		case WA_INACTIVE:
			if (false == s_windowed)
			{
				if (nullptr != DXGI::GetSwapChain())
				{
					// Push window to bottom of the Z order.
					SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
				}
			}

			s_wndIsActive = false;
			break;
		};

	case WM_SIZE:
		// ALT+ENTER is blocked, all else is ignored or automatically scaled if the window type permits it.
		break;
	}

	// We're not handling this message, so pass it down the chain of command.
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static bool CreateAppWindow(HINSTANCE hInstance, int nCmdShow)
{
	// Define window class.
	// This is basically a template that describes a few of a window's basic properties & associations.
	WNDCLASSEX wndClass;
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = 0;
	wndClass.lpfnWndProc = WindowProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hInstance;
	wndClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wndClass.hCursor = NULL;
	wndClass.hbrBackground = (s_windowed) ? (HBRUSH)GetStockObject(BLACK_BRUSH) : NULL;
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = APP_ID.c_str();
	wndClass.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));

	// Register window class.
	if (0 == RegisterClassEx(&wndClass))
	{
		SetLastError("Can not create application window (RegisterClassEx() failed).");
		return false;
	}

	s_classRegged = true;

	// Figure out style flags.
	DWORD windowStyle, exWindowStyle;
	if (true == s_windowed)
	{
		// Windowed style.
		windowStyle = WS_POPUP | WS_CAPTION | WS_SYSMENU;
		exWindowStyle = 0;
	}
	else
	{
		// Full screen style (WS_EX_TOPMOST assigned by WM_ACTIVATE).
		windowStyle = WS_POPUP;
		exWindowStyle = 0;
	}

	// Calculate full window size based on our required client area.
	const DXGI_MODE_DESC modeDesc = DXGI::GetDisplayMode();
	RECT wndRect = { 0, 0, (LONG) modeDesc.Width, (LONG) modeDesc.Height }; // Ugly casts due to 32-bit legacy.
	AdjustWindowRectEx(&wndRect, windowStyle, FALSE, exWindowStyle);
	const int wndWidth = wndRect.right - wndRect.left;
	const int wndHeight = wndRect.bottom - wndRect.top;

	// Then finally: create it.
	s_hWnd = CreateWindowEx(
		exWindowStyle,
		APP_ID.c_str(),
		APP_TITLE.c_str(),
		windowStyle,
		0, 0, // Always pop up on primary display's desktop area (*).
		wndWidth, wndHeight,
		NULL,
		NULL,
		hInstance,
		nullptr);

	// * - Works fine in any windowed mode and it's automatically moved if necessary for
	//     full screen rendering.

	if (NULL == s_hWnd)
	{
		SetLastError("Can not create application window (CreateWindowEx() failed).");
		return false;
	}

	ShowWindow(s_hWnd, (s_windowed) ? nCmdShow : SW_SHOW);

	return true;
}

static bool UpdateAppWindow(bool &renderFrame)
{
	// Skip rendering this frame, unless otherwise specified.
	renderFrame = false;

	// Got a message to process?
	MSG msg;
	if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			// We've been ordered to abandon ship.
			return false;
		}

		// Dispatch message to handler (WindowProc()).
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	else
	{
		// Window still alive?
		if (NULL != s_hWnd)
		{
			if (false == s_windowed && true == s_wndIsActive)
			{
				// Kill cursor for active full screen window.
				SetCursor(NULL);
			}

			// Render frame if windowed or full screen window has focus.
			if (s_windowed || s_wndIsActive)
			{
				renderFrame = true;
			}
			else
			{
				// Full screen window out of focus: relinquish rest of time slice.
				// This keeps your loop from spinning (and spiking a core up to 100%).
				Sleep(0);
			}
		}
	}

	// Continue!
	return true;
}

void DestroyAppWindow(HINSTANCE hInstance)
{
	if (NULL != s_hWnd)
	{
		// Destroy window.
		DestroyWindow(s_hWnd);
		s_hWnd = NULL;
	}

	if (true == s_classRegged)
	{
		// Unregister it's class.
		UnregisterClass(L"RenderWindow", hInstance);
	}
}

// Our own entry point (taken care off in the function below).
int __stdcall Main(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int nCmdShow)
{


/*
	// Check for SSE4.1
	int cpuInfo[4];
	__cpuid(cpuInfo, 1);
	if (0 == (cpuInfo[2] & CPUID_FEAT_ECX_SSE4_1))
	{
		MessageBox(NULL, L"System does not support SSE4.1 instructions.", PLAYER_RELEASE_TITLE.c_str(), MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}
*/

	// Initialize DXGI.
	if (DXGI::Create(hInstance, s_windowed))
	{
#if (!defined(_DEBUG) && !defined(_DESIGN)) || defined(FORCE_SETUP_DIALOG)
		
		// At this point DXGI is ready to use the primary display and for most cases that's just what I need.
		// However, the setup dialog allows you to pick a mode (windowed or full screen), display adapter
		// and display mode. In the past games often cames with dialogs like these, nowadays it's more often
		// stashed inside the in-game settings.
		// Any additional DXGI/Win32 logic is handled in SetupDialog.cpp itself!

		// These variables are passed by reference.	
		// Not great, but I can't be bothered to change it now.
		unsigned int iAdapter, iOutput;
		DXGI_MODE_DESC dispMode;
		float aspectRatio;
		unsigned int multiSamples;
		bool windowed;
		bool vSync;

		if (true == SetupDialog(hInstance, iAdapter, iOutput, dispMode, aspectRatio, multiSamples, windowed, vSync, DXGI::GetFactory()))
		{
			s_windowed = windowed;

			if (false == s_windowed) // In windowed mode we'll use the primary adapter (display is irrelevant).
			{
				// Override with user-selected settings.
				DXGI::OverrideAdapterAndDisplayMode(iAdapter, iOutput, dispMode);
			}
#else
		if (true) // Complete the scope.
		{
			const bool vSync = VSYNC_DEV;                 // Dev. toggle.
			float aspectRatio = -1.f;                     // Automatic mode.
			unsigned int multiSamples = MULTI_SAMPLE_DEV; // Dev. multi-sampling.

			// Other variables are already set up correctly.
#endif
			// Create render window.
			if (CreateAppWindow(hInstance, nCmdShow))
			{
				DXGI_SAMPLE_DESC multiDesc;
				if (multiSamples <= 1)
				{
					// Turn multi-sampling off.
					multiDesc.Count = 1;
					multiDesc.Quality = 0;
				}
				else
				{
					// According to Direct3D 11 spec. 1 (does not apply here), 2, 4 and 8 must be supported.
					// As I don't have (nor intend to) have an actual device at this point I can't query (FIXME?).
					ASSERT(multiSamples == 2 || multiSamples == 4 || multiSamples == 8);
					multiDesc.Count = multiSamples;
					multiDesc.Quality = D3D11_STANDARD_MULTISAMPLE_PATTERN;
				}

				// Initialize Direct3D 11 (a DXGI task).
				if (DXGI::CreateDevice(s_hWnd, s_windowed, multiDesc))
				{
					if (-1.f == aspectRatio) // Anything special?
					{
						// Derive aspect ratio from resolution (square pixels).
						const DXGI_MODE_DESC modeDesc = DXGI::GetDisplayMode();
						aspectRatio = (float)modeDesc.Width / modeDesc.Height;
					}

					// Initialize D3D renderer.
					if (D3D::Create(DXGI::GetDevice(), DXGI::GetContext(), DXGI::GetSwapChain(), multiDesc, RENDER_ASPECT_RATIO, aspectRatio))
					{
						// In windowed mode FPS counter is refreshed every 60 frames.
						Timer timer;
						float timeElapsedFPS = 0.f;
						unsigned int numFramesFPS = 0;
						float prevTimeElapsed = timer.Get();

						// Enter (render) loop.
						bool renderFrame;
						while (true == UpdateAppWindow(renderFrame))
						{
							if (true == renderFrame)
							{
								// Render frame.
								const float time = timer.Get();
								const float timeElapsed = time - prevTimeElapsed;
								prevTimeElapsed = time;

								D3D::RenderFrame();

								// Desktop ignores vertical sync., otherwise sync. to refresh rate (usually 60Hz).
								D3D::Flip((true == s_windowed) ? false : true == vSync);

								if (true == s_windowed)
								{
									// Handle FPS counter.
									timeElapsedFPS += timeElapsed;

									if (++numFramesFPS == 60)
									{
										const float FPS = 60.f / timeElapsedFPS;

										wchar_t fpsStr[256];
										swprintf(fpsStr, 256, L"%s (%2f FPS)", APP_TITLE.c_str(), FPS);
										SetWindowText(s_hWnd, fpsStr);

										timeElapsedFPS = 0.f;
										numFramesFPS = 0;
									}
								}
							} // true == renderFrame
						}
					}
				}
			}
		} // SetupDialog()
	}

	// Destroy resources in reverse order.
	D3D::Destroy();
	DXGI::DestroyDevice(s_windowed);
	DestroyAppWindow(hInstance);
	DXGI::Destroy();

	// Got an error to report?
	if (false == s_lastError.empty())
	{
		MessageBox(NULL, ToUnicode(s_lastError).c_str(), APP_ID.c_str(), MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}

	return 0;
}

// The actual Win32 entry point.
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int nCmdShow)
{
#if defined(_DEBUG)
	// Dump leak report at any possible exit.
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	// Report all to debug pane.
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);

	// Set alloc. break (optional).
	if (-1 != PLAYER_CRT_BREAK_ALLOC)
		_CrtSetBreakAlloc(PLAYER_CRT_BREAK_ALLOC);
#endif

	const int result = Main(hInstance, hPrevInstance, cmdLine, nCmdShow);
	return result;

#if !defined(_DEBUG) && !defined(_DESIGN)
}
__except (EXCEPTION_EXECUTE_HANDLER)
{
	// Attempt to restore the desktop (just killing DXGI should do the trick).
	DXGI::Destroy();
	if (NULL != s_hWnd) DestroyWindow(s_hWnd);

	// Sound the alarm bell.
	MessageBox(NULL, L"Application crashed due to an unhandled exception.",  APP_ID.c_str(), MB_OK | MB_ICONEXCLAMATION);

	// Better do as little as possible past this point.
	_exit(1);
}
#endif

	return 1;
}
