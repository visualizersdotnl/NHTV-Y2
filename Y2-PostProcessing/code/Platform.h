
/*
	Year 2 Direct3D 11 workshop template.
	Platform.h - Global platform include (generally goes on top, everywhere, much like a PCH).

	- API, CRT, STL & a few local essentials.
	- A few (very) basic macros and functions.
*/

#if !defined(PLATFORM_H)
#define PLATFORM_H

// To enable CRT leak dump & trace for debug builds.
#if defined(_DEBUG)
	#define CRTDBG_MAP_ALLOC  
	#include <stdlib.h>  
	#include <crtdbg.h>
#endif

// APIs
#include <windows.h>
#include <dxgi.h>
#include <d3d11.h>
#include <d3d11shader.h>
// #include <DirectXMath.h>
// #include <DirectXPackedVector.h>

// Link against these Windows SDK libraries:
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")

// CRT & STL
#include <stdint.h>
#include <string>
#include <iostream>
#include <sstream>
#include <array>
#include <vector>
#include <list>
#include <algorithm>
#include <exception>
#include <memory>

// Local
#include "../3rdparty/Std3DMath/Math.h"
#include "platform/Assert.h"
#include "platform/Noncopyable.h"
#include "platform/ComPtr.h"
#include "platform/DebugLog.h"
#include "platform/Timer.h"
#include "platform/StringUtil.h"

// For easy access to DirectXMath types.
// using namespace DirectX;
// using namespace DirectX::PackedVector;

// See (on top of) Win32.cpp (important!)
extern void SetLastError(const std::string &message);

inline bool IsPowerOfTwo(unsigned int X)
{
	return (0 != X) && (X & (~X + 1)) == X;
}

#endif // PLATFORM_H
