
/*
	Year 2 Direct3D 11 workshop template.
	Settings.h - Global application settings.
*/

#if !defined(SETTINGS_H)
#define SETTINGS_H

// Break on allocation number (use with leak detection in debug build; -1 for none).
const int PLAYER_CRT_BREAK_ALLOC = -1;

// ID & title.
const std::wstring APP_ID = L"Y2-D3D11";
const std::wstring APP_TITLE = L"Post-processing";

// Render aspect ratio.
// const float RENDER_ASPECT_RATIO = 4.f / 3.f; // Old school.
const float RENDER_ASPECT_RATIO = 16.f/9.f;  // Widescreen.

// Dev. settings (can be bypassed by the setup dialog):
const bool WINDOWED_DEV = true;  // Windowed or full screen.
const bool VSYNC_DEV = true;     // Vertical sync.
const UINT MULTI_SAMPLE_DEV = 1; // Multi-sampling: 1 means OFF, otherwise pick: 2, 4 or 8 (samples).

// Windowed resolution (fixed).
const unsigned int WINDOWED_RES_X = 1280;
const unsigned int WINDOWED_RES_Y = 720;

// In debug & design builds the dialog is skipped, except when FORCE_SETUP_DIALOG is defined.
#define FORCE_SETUP_DIALOG

// Back buffer format (regular & gamma-corrected).
const DXGI_FORMAT D3D_BACK_BUFFER_FORMAT = DXGI_FORMAT_B8G8R8A8_UNORM;
const DXGI_FORMAT D3D_BACK_BUFFER_FORMAT_GAMMA = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

// Set to 'true' to have D3D shut it's trap about lingering binds (applies to debug build only).
const bool D3D_DISABLE_SPECIFIC_WARNINGS = true; // Unused (FIXME)

#endif
