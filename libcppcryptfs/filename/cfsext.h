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

// CfsExt (feature flag "CfsExt", only with PlaintextNames): directories keep
// their names in the cipher dir, every other entry is stored as NAME.cfs (the
// suffix is written in lower case and matched ASCII case-insensitively).
// Pure helpers on Dokan-style paths ("\dir\name:stream"), see cfsext_test.cpp.

#define CFSEXT_SUFFIX L".cfs"
#define CFSEXT_SUFFIX_LEN (sizeof(CFSEXT_SUFFIX) / sizeof(wchar_t) - 1)

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

	stream = (colon == std::wstring::npos) ? std::wstring() : last.substr(colon);
	name = last.substr(0, colon);
}

// "name:stream" -> "name.cfs:stream"
inline std::wstring cfsext_add(const std::wstring& last)
{
	std::wstring name, stream;

	cfsext_split_stream(last, name, stream);

	return name + CFSEXT_SUFFIX + stream;
}

// "name.cfs:stream" -> "name:stream"; false if the name part is not suffixed
// or would be empty, "." or ".." without the suffix
inline bool cfsext_strip(const std::wstring& last, std::wstring& out)
{
	std::wstring name, stream;

	cfsext_split_stream(last, name, stream);

	if (!cfsext_is_suffixed(name))
		return false;

	name.erase(name.size() - CFSEXT_SUFFIX_LEN);

	if (name.empty() || name == L"." || name == L"..")
		return false;

	out = name + stream;

	return true;
}

// true if a backslash-separated component of path other than the last one
// ends in ".cfs" (it would have to be a directory, and directories cannot be
// named *.cfs); empty components (leading or trailing backslash) are ignored
inline bool cfsext_bad_intermediate(const std::wstring& path)
{
	const std::wstring::size_type end = path.find_last_not_of(L'\\');

	for (std::wstring::size_type i = path.find(L'\\'); i != std::wstring::npos && i < end; i = path.find(L'\\', i + 1))
		if (cfsext_is_suffixed(path.substr(0, i)))
			return true;

	return false;
}

// the part of path after its last backslash (empty for the root "\" or a
// path ending in a backslash, the whole path if it has no backslash)
inline std::wstring cfsext_last_component(const std::wstring& path)
{
	const std::wstring::size_type slash = path.rfind(L'\\');

	return slash == std::wstring::npos ? path : path.substr(slash + 1);
}

// the name part (without any ":stream") of the last component of path,
// ignoring one trailing backslash; like get_file_stream(), the stream is split
// off at the first colon of the whole path (so "\f\:s" gives "f"), which is
// why path must not start with a drive letter
inline std::wstring cfsext_last_name(const std::wstring& path)
{
	std::wstring file = path.substr(0, path.find(L':'));

	if (file.size() > 1 && file[file.size() - 1] == L'\\')
		file.erase(file.size() - 1);

	return cfsext_last_component(file);
}
