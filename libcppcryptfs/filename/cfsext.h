/*
cppcryptfs : user-mode cryptographic virtual overlay filesystem.

Copyright (C) 2016-2026 Bailey Brown (github.com/bailey27/cppcryptfs)

cppcryptfs is based on the design of gocryptfs (github.com/rfjakob/gocryptfs)

The MIT License (MIT)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#pragma once

#include <string>

// CfsExt (feature flag "CfsExt", only together with PlaintextNames):
// directories keep their names in the cipher dir, every other entry is
// stored as NAME.cfs.  The suffix is written in lower case and matched
// ASCII case-insensitively, because NTFS is case-insensitive.
//
// These are pure helpers on Dokan-style paths ("\dir\name:stream") without
// any Windows types, so that they can be unit tested on Linux (see
// cfsext_test.cpp).

// An inline variable needs C++17.  Without it (MSVC has defaulted to
// /std:c++14 and the vcxproj files set no LanguageStandard), every
// translation unit gets its own copy.
#ifdef __cpp_inline_variables
inline const wchar_t CFSEXT_SUFFIX[] = L".cfs";
#else
static const wchar_t CFSEXT_SUFFIX[] = L".cfs";
#endif

const std::wstring::size_type CFSEXT_SUFFIX_LEN = sizeof(CFSEXT_SUFFIX) / sizeof(CFSEXT_SUFFIX[0]) - 1;

// true if name ends in ".cfs" (ASCII case-insensitively; ".cfs" alone is
// suffixed, too)
inline bool cfsext_is_suffixed(const std::wstring& name)
{
	if (name.size() < CFSEXT_SUFFIX_LEN)
		return false;

	const std::wstring::size_type off = name.size() - CFSEXT_SUFFIX_LEN;

	for (std::wstring::size_type i = 0; i < CFSEXT_SUFFIX_LEN; i++) {
		wchar_t c = name[off + i];
		if (c >= L'A' && c <= L'Z')
			c = static_cast<wchar_t>(c - L'A' + L'a');
		if (c != CFSEXT_SUFFIX[i])
			return false;
	}

	return true;
}

// splits the last path component at its first colon into the name and the
// stream part (":stream" or ":stream:$DATA", including the colon; empty if
// there is no stream)
inline void cfsext_split_stream(const std::wstring& last, std::wstring& name, std::wstring& stream)
{
	const std::wstring::size_type colon = last.find(L':');

	if (colon == std::wstring::npos) {
		name = last;
		stream.clear();
	} else {
		std::wstring s = last.substr(colon);
		name = last.substr(0, colon);
		stream.swap(s);
	}
}

// "name:stream" -> "name.cfs:stream"
inline std::wstring cfsext_add(const std::wstring& last)
{
	std::wstring name, stream;

	cfsext_split_stream(last, name, stream);

	return name + CFSEXT_SUFFIX + stream;
}

// "name.cfs:stream" -> "name:stream"; false if the name part is not suffixed
// or would be empty without the suffix
inline bool cfsext_strip(const std::wstring& last, std::wstring& out)
{
	std::wstring name, stream;

	cfsext_split_stream(last, name, stream);

	if (!cfsext_is_suffixed(name) || name.size() == CFSEXT_SUFFIX_LEN)
		return false;

	out = name.substr(0, name.size() - CFSEXT_SUFFIX_LEN) + stream;

	return true;
}

// true if a backslash-separated component of path other than the last one
// ends in ".cfs" (it would have to be a directory, and directories cannot be
// named *.cfs); empty components (leading or trailing backslash) are ignored
inline bool cfsext_bad_intermediate(const std::wstring& path)
{
	std::wstring::size_type end = path.size();

	while (end > 0 && path[end - 1] == L'\\')
		end--;

	std::wstring::size_type start = 0;

	for (;;) {
		const std::wstring::size_type slash = path.find(L'\\', start);
		if (slash == std::wstring::npos || slash >= end)
			return false; // the rest is the last component
		if (cfsext_is_suffixed(path.substr(start, slash - start)))
			return true;
		start = slash + 1;
	}
}

// the part of path after its last backslash (empty for the root "\" or a
// path ending in a backslash, the whole path if it has no backslash)
inline std::wstring cfsext_last_component(const std::wstring& path)
{
	const std::wstring::size_type slash = path.rfind(L'\\');

	return slash == std::wstring::npos ? path : path.substr(slash + 1);
}
