// Linux-only unit test: g++ -std=c++17 -Wall -Wextra cfsext_test.cpp && ./a.out
//
// Tests the pure CfsExt helpers in cfsext.h.  Not part of any Visual Studio
// project.

#undef NDEBUG
#include <cassert>
#include <cstdio>
#include <string>

#include "cfsext.h"

static void test_is_suffixed()
{
	assert(std::wstring(CFSEXT_SUFFIX) == L".cfs");

	assert(cfsext_is_suffixed(L"foo.cfs"));
	assert(cfsext_is_suffixed(L"foo.CFS"));
	assert(cfsext_is_suffixed(L"foo.cFs"));
	assert(cfsext_is_suffixed(L".cfs"));

	assert(!cfsext_is_suffixed(L""));
	assert(!cfsext_is_suffixed(L"cfs"));
	assert(!cfsext_is_suffixed(L"foo_cfs"));
	assert(!cfsext_is_suffixed(L"foo.cfs.txt"));
	assert(!cfsext_is_suffixed(L"foo.cfs:s")); // callers split off the stream first
}

static void test_split_stream()
{
	std::wstring name, stream;

	cfsext_split_stream(L"foo.txt", name, stream);
	assert(name == L"foo.txt" && stream.empty());

	cfsext_split_stream(L"foo.txt:s:$DATA", name, stream);
	assert(name == L"foo.txt" && stream == L":s:$DATA");

	cfsext_split_stream(L":s", name, stream);
	assert(name.empty() && stream == L":s");
}

static void test_add_strip()
{
	std::wstring out;

	assert(cfsext_add(L"foo.txt") == L"foo.txt.cfs");
	assert(cfsext_add(L"foo.txt:s") == L"foo.txt.cfs:s");
	assert(cfsext_add(L"x.cfs") == L"x.cfs.cfs");

	assert(cfsext_strip(L"foo.txt.cfs", out) && out == L"foo.txt");
	assert(cfsext_strip(L"foo.txt.CFS:s:$DATA", out) && out == L"foo.txt:s:$DATA");
	assert(cfsext_strip(L"x.cfs.cfs", out) && out == L"x.cfs");
	assert(cfsext_strip(cfsext_add(L"foo.txt:s"), out) && out == L"foo.txt:s");

	// not suffixed: false, out unchanged
	out = L"unchanged";
	assert(!cfsext_strip(L"foo.txt", out) && out == L"unchanged");
	assert(!cfsext_strip(L"foo.txt:s.cfs", out) && out == L"unchanged");

	// empty, "." or ".." without the suffix: false
	assert(!cfsext_strip(L".cfs", out) && out == L"unchanged");
	assert(!cfsext_strip(L"..cfs", out) && out == L"unchanged");
	assert(!cfsext_strip(L"...cfs", out) && out == L"unchanged");
	assert(cfsext_strip(L"....cfs", out) && out == L"...");
}

static void test_bad_intermediate()
{
	assert(cfsext_bad_intermediate(L"\\x.cfs\\f"));
	assert(cfsext_bad_intermediate(L"\\a\\x.cfs\\f.cfs"));

	assert(!cfsext_bad_intermediate(L"\\x\\f.cfs"));
	assert(!cfsext_bad_intermediate(L"\\x.cfs"));
	assert(!cfsext_bad_intermediate(L"\\x.cfs\\"));
	assert(!cfsext_bad_intermediate(L"\\a\\x.cfs\\"));
	assert(!cfsext_bad_intermediate(L"\\"));
	assert(!cfsext_bad_intermediate(L""));
}

static void test_last_component()
{
	assert(cfsext_last_component(L"\\") == L"");
	assert(cfsext_last_component(L"\\a\\b") == L"b");
	assert(cfsext_last_component(L"\\a\\") == L"");
	assert(cfsext_last_component(L"\\a\\b.txt:s") == L"b.txt:s");
	assert(cfsext_last_component(L"name") == L"name");
}

static void test_last_name()
{
	assert(cfsext_last_name(L"\\dir\\f:s") == L"f");
	assert(cfsext_last_name(L"\\dir\\f\\") == L"f");
	assert(cfsext_last_name(L"\\f\\:s") == L"f"); // Dokan quirk: stream after a backslash
	assert(cfsext_last_name(L"\\") == L"");
	assert(cfsext_last_name(L"\\:s") == L"");
}

int main()
{
	test_is_suffixed();
	test_split_stream();
	test_add_strip();
	test_bad_intermediate();
	test_last_component();
	test_last_name();

	printf("cfsext_test: all tests passed\n");

	return 0;
}
