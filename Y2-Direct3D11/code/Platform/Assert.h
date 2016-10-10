
/*
	ASSERT/VERIFY/ASSERT_MSG.
	Deviates from CRT assertions, like most real-world cases do.
*/

#pragma once

#if defined(_DEBUG)
	#define ASSERT(condition) if (!(condition)) __debugbreak();
	#define VERIFY(condition) ASSERT(condition)
	#define ASSERT_MSG(condition, message) if (!(condition)) { std::cout << message << "\n"; __debugbreak(); }
#else
	#define ASSERT(condition)
	#define VERIFY(condition) (condition)
	#define ASSERT_MSG(condition, message)
#endif
