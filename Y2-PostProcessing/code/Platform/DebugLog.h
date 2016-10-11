
#pragma once

#if defined(_DEBUG) || defined(_DESIGN)
	// Note: issue lines separately; 384 characters per line seems lenient enough.
	#define DEBUG_LOG(format, ...) { char string[384]; sprintf_s(string, 384, format, __VA_ARGS__); std::cout << string << "\n"; }
#else
	#define DEBUG_LOG(format, ...) 
#endif 
