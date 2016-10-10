
/*
	Aliasing Microsoft WRL::ComPtr (in the global namespace).
*/

#pragma once

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

// DEPRECATED -- use ComPtr instead of raw pointer.
#define SAFE_RELEASE(pCOM) if (nullptr != (pCOM)) (pCOM)->Release()
