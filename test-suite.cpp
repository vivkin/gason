#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if !defined(_WIN32) && !defined(NDEBUG)
#include <execinfo.h>
#include <signal.h>
#endif
#include "gason.h"

const char *SUITE[] =
{
R"raw("A JSON payload should be an object or array, not a string.")raw"
R"raw(["Unclosed array")raw",
R"raw({unquoted_key: "keys must be quoted"})raw",
R"raw(["extra comma",])raw",
R"raw(["double extra comma",,])raw",
R"raw([   , "<-- missing value"])raw",
R"raw(["Comma after the close"],)raw",
R"raw(["Extra close"]])raw",
R"raw({"Extra comma": true,})raw",
R"raw({"Extra value after close": true} "misplaced quoted value")raw",
R"raw({"Illegal expression": 1 + 2})raw",
R"raw({"Illegal invocation": alert()})raw",
R"raw({"Numbers cannot have leading zeroes": 013})raw",
R"raw({"Numbers cannot be hex": 0x14})raw",
R"raw(["Illegal backslash escape: \x15"])raw",
R"raw([\naked])raw",
R"raw(["Illegal backslash escape: \017"])raw",
R"raw([[[[[[[[[[[[[[[[[[[["Too deep"]]]]]]]]]]]]]]]]]]]])raw",
R"raw({"Missing colon" null})raw",
R"raw({"Double colon":: null})raw",
R"raw({"Comma instead of colon", null})raw",
R"raw(["Colon instead of comma": false])raw",
R"raw(["Bad value", truth])raw",
R"raw(['single quote'])raw",
R"raw(["	tab	character	in	string	"])raw",
R"raw(["tab\   character\   in\  string\  "])raw",
R"raw(["line
eak"])raw",
R"raw(["line\
eak"])raw",
R"raw([0e])raw",
R"raw([0e+])raw",
R"raw([0e+-1])raw",
R"raw({"Comma instead if closing brace": true,)raw",
R"raw(["mismatch"})raw",
R"raw(
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
,"rosebud"])raw",
R"raw([[[[[[[[[[[[[[[[[[["Not too deep"]]]]]]]]]]]]]]]]]]])raw",
R"raw({
    "JSON Test Pattern pass3": {
        "The outermost value": "must be an object or array.",
        "In this test": "It is an object."
    }
}
)raw",
R"raw([1, 2, "хУй", [[0.5], 7.11, 13.19e+1], "ba\u0020r", [ [ ] ], -0, -.666, [true, null], {"WAT?!": false}])raw"
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
			fprintf(stderr, "%s\n", names[i]);
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
	for (auto *s : SUITE)
	{
		char *source = strdup(s);
		JsonParseStatus status = json_parse(source, &endptr, &value, allocator);
		if (status != JSON_PARSE_OK)
		{
			int error_pos = endptr - source + 1;
			fprintf(stderr, "test-suite%d:%d: error: %d, %.*s<--- there\n", count, error_pos, (int)status, error_pos, s);
		}
		else
		{
			++passed;
		}
		free(source);
		++count;
	}
	fprintf(stderr, "%d/%d\n", passed, count);

	return 0;
}
