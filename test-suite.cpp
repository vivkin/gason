#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if !defined(_WIN32) && !defined(NDEBUG)
#include <execinfo.h>
#include <signal.h>
#endif
#ifdef ANDROID
#include <android/log.h>
#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "ruberoid", __VA_ARGS__)
#else
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#endif
#include "gason.h"

struct { bool f; const char *s; } SUITE[] = {
{false, R"*(1234567890)*"},
{false, R"*("A JSON payload should be an object or array, not a string.")*"},
{true, R"*(["Unclosed array")*"},
{true, R"*({unquoted_key: "keys must be quoted"})*"},
{false, R"*(["extra comma",])*"},
{true, R"*(["double extra comma",,])*"},
{true, R"*([   , "<-- missing value"])*"},
{false, R"*(["Comma after the close"],)*"},
{false, R"*({"Extra comma": true,})*"},
{false, R"*({"Extra value after close": true} "misplaced quoted value")*"},
{true, R"*({"Illegal expression": 1 + 2})*"},
{true, R"*({"Illegal invocation": alert()})*"},
{false, R"*({"Numbers cannot have leading zeroes": 013})*"},
{true, R"*({"Numbers cannot be hex": 0x14})*"},
{true, R"*(["Illegal backslash escape: \x15"])*"},
{true, R"*([\naked])*"},
{true, R"*(["Illegal backslash escape: \017"])*"},
{true, R"*([[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[["Too deep"]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]])*"},
{false, R"*({"Missing colon" null})*"},
{true, R"*({"Double colon":: null})*"},
{true, R"*({"Comma instead of colon", null})*"},
{true, R"*(["Colon instead of comma": false])*"},
{true, R"*(["Bad value", truth])*"},
{true, R"*(['single quote'])*"},
{false, R"*(["	tab	character	in	string	"])*"},
{false, R"*(["line
break"])*"},
{true, R"*(["line\
break"])*"},
{false, R"*([0e])*"},
{false, R"*([0e+])*"},
{true, R"*([0e+-1])*"},
{true, R"*({"Comma instead if closing brace": true,)*"},
{true, R"*(["mismatch"})*"},
{false, R"*(
[
    "JSON Test Pattern pass1",
    {"object with 1 member":["array with 1 element"]},
    {},
    [],
    -42,
    true,
    false,
    null,
    {
        "integer": 1234567890,
        "real": -9876.543210,
        "e": 0.123456789e-12,
        "E": 1.234567890E+34,
        "":  23456789012E66,
        "zero": 0,
        "one": 1,
        "space": " ",
        "quote": "\"",
        "backslash": "\\",
        "controls": "\b\f\n\r\t",
        "slash": "/ & \/",
        "alpha": "abcdefghijklmnopqrstuvwyz",
        "ALPHA": "ABCDEFGHIJKLMNOPQRSTUVWYZ",
        "digit": "0123456789",
        "0123456789": "digit",
        "special": "`1~!@#$%^&*()_+-={':[,]}|;.</>?",
        "hex": "\u0123\u4567\u89AB\uCDEF\uabcd\uef4A",
        "true": true,
        "false": false,
        "null": null,
        "array":[  ],
        "object":{  },
        "address": "50 St. James Street",
        "url": "http://www.JSON.org/",
        "comment": "// /* <!-- --",
        "# -- --> */": " ",
        " s p a c e d " :[1,2 , 3

,

4 , 5        ,          6           ,7        ],"compact":[1,2,3,4,5,6,7],
        "jsontext": "{\"object with 1 member\":[\"array with 1 element\"]}",
        "quotes": "&#34; \u0022 %22 0x22 034 &#x22;",
        "\/\\\"\uCAFE\uBABE\uAB98\uFCDE\ubcda\uef4A\b\f\n\r\t`1~!@#$%^&*()_+-=[]{}|;:',./<>?"
: "A key can be any string"
    },
    0.5 ,98.6
,
99.44
,

1066,
1e1,
0.1e1,
1e-1,
1e00,2e+00,2e-00
,"rosebud"])*"},
{false, R"*([[[[[[[[[[[[[[[[[[["Not too deep"]]]]]]]]]]]]]]]]]]])*"},
{false, R"*({
    "JSON Test Pattern pass3": {
        "The outermost value": "must be an object or array.",
        "In this test": "It is an object."
    }
}
)*"},
{false, R"*([1, 2, "хУй", [[0.5], 7.11, 13.19e+1], "ba\u0020r", [ [ ] ], -0, -.666, [true, null], {"WAT?!": false}])*"},
};

int main()
{
#if !defined(_WIN32) && !defined(NDEBUG)
	auto abort_handler = [](int)
	{
		void *callstack[16];
		int size = backtrace(callstack, sizeof(callstack) / sizeof(callstack[0]));
		char **names = backtrace_symbols(callstack, size);
		for (int i = 0; i < size; ++i)
		{
			LOG("%s\n", names[i]);
		}
		free(names);
		exit(EXIT_FAILURE);
	};
	signal(SIGABRT, abort_handler);
#endif

	char *endptr;
	JsonValue value;
	JsonAllocator allocator;
	int passed = 0;
	int count = 0;
	for (auto t : SUITE)
	{
		char *source = strdup(t.s);
		JsonParseStatus status = json_parse(source, &endptr, &value, allocator);
		free(source);
		if (t.f ^ (status == JSON_PARSE_OK))
		{
			++passed;
		}
		else
		{
			LOG("%d: must be %s: %s\n", count, t.f ? "fail" : "pass", t.s);
		}
		++count;
	}
	LOG("%d/%d\n", passed, count);

	return 0;
}
