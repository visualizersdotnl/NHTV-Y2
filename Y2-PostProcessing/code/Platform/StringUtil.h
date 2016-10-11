
/*
	A few string utility functions (hashing & Unicode conversion).
*/

#pragma once

#include <wchar.h>
#include <locale>
#include <codecvt>

// http://stackoverflow.com/questions/98153/whats-the-best-hashing-algorithm-to-use-on-a-stl-string-when-using-hash-map
inline unsigned int StringHash(const std::string& path, unsigned int seed)
{
	unsigned int hash = seed;
	for (auto character : path)
		hash = hash*101 + character;

	return hash;
}

/*
	ToUnicode() & ToNarrow() are necessary in a few places as the internal convention is C string/UTF-8,
	but we compile and communicate as a Unicode application.

	see: http://stackoverflow.com/questions/2573834/c-convert-string-or-char-to-wstring-or-wchar-t
*/

inline std::wstring ToUnicode(const std::string& string)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(string);
}

inline std::string ToMultibyte(const std::wstring& string)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.to_bytes(string);
}
