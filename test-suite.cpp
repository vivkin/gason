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
u8R"raw("A JSON payload should be an object or array, not a string.")raw",
u8R"raw(["Unclosed array")raw",
u8R"raw({unquoted_key: "keys must be quoted"})raw",
u8R"raw(["extra comma",])raw",
u8R"raw(["double extra comma",,])raw",
u8R"raw([   , "<-- missing value"])raw",
u8R"raw(["Comma after the close"],)raw",
u8R"raw(["Extra close"]])raw",
u8R"raw({"Extra comma": true,})raw",
u8R"raw({"Extra value after close": true} "misplaced quoted value")raw",
u8R"raw({"Illegal expression": 1 + 2})raw",
u8R"raw({"Illegal invocation": alert()})raw",
u8R"raw({"Numbers cannot have leading zeroes": 013})raw",
u8R"raw({"Numbers cannot be hex": 0x14})raw",
u8R"raw(["Illegal backslash escape: \x15"])raw",
u8R"raw([\naked])raw",
u8R"raw(["Illegal backslash escape: \017"])raw",
u8R"raw([[[[[[[[[[[[[[[[[[[["Too deep"]]]]]]]]]]]]]]]]]]]])raw",
u8R"raw({"Missing colon" null})raw",
u8R"raw({"Double colon":: null})raw",
u8R"raw({"Comma instead of colon", null})raw",
u8R"raw(["Colon instead of comma": false])raw",
u8R"raw(["Bad value", truth])raw",
u8R"raw(['single quote'])raw",
u8R"raw(["	tab	character	in	string	"])raw",
u8R"raw(["tab\   character\   in\  string\  "])raw",
u8R"raw(["line
break"])raw",
u8R"raw(["line\
break"])raw",
u8R"raw([0e])raw",
u8R"raw([0e+])raw",
u8R"raw([0e+-1])raw",
u8R"raw({"Comma instead if closing brace": true,)raw",
u8R"raw(["mismatch"})raw",
u8R"raw(
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
u8R"raw([[[[[[[[[[[[[[[[[[["Not too deep"]]]]]]]]]]]]]]]]]]])raw",
u8R"raw({
    "JSON Test Pattern pass3": {
        "The outermost value": "must be an object or array.",
        "In this test": "It is an object."
    }
}
)raw",
u8R"raw([1, 2, "хУй", [[0.5], 7.11, 13.19e+1], "ba\u0020r", [ [ ] ], -0, -.666, [true, null], {"WAT?!": false}])raw"
};

void print_error(const char *filename, JsonParseStatus status, char *endptr, char *source, size_t size)
{
	const char *status2str[] = {
		"JSON_PARSE_OK",
		"JSON_PARSE_BAD_NUMBER",
		"JSON_PARSE_BAD_STRING",
		"JSON_PARSE_BAD_IDENTIFIER",
		"JSON_PARSE_STACK_OVERFLOW",
		"JSON_PARSE_STACK_UNDERFLOW",
		"JSON_PARSE_MISMATCH_BRACKET",
		"JSON_PARSE_UNEXPECTED_CHARACTER",
		"JSON_PARSE_UNQUOTED_KEY",
		"JSON_PARSE_BREAKING_BAD"
	};
	char *s = endptr;
	while (s != source && *s != '\n') --s;
	if (s != endptr && s != source) ++s;
	int line = 0;
	for (char *i = s; i != source; --i)
	{
		if (*i == '\n')
		{
			++line;
		}
	}
	int column = (int)(endptr - s);
	fprintf(stderr, "%s:%d:%d: error %s\n", filename, line + 1, column + 1, status2str[status]);
	while (s != source + size && *s != '\n')
	{
		int c = *s++;
		switch (c)
		{
			case '\b': fprintf(stderr, "\\b"); column += 1; break;
			case '\f': fprintf(stderr, "\\f"); column += 1; break;
			case '\n': fprintf(stderr, "\\n"); column += 1; break;
			case '\r': fprintf(stderr, "\\r"); column += 1; break;
			case '\t': fprintf(stderr, "%*s", 4, ""); column += 3; break;
			case 0: fprintf(stderr, "\""); break;
			default: fprintf(stderr, "%c", c);
		}
	}
	fprintf(stderr, "\n%*s\n", column + 1, "^");
}

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
	size_t passed = 0;
	for (auto *s : SUITE)
	{
		size_t length = strlen(s);
		char *source = strdup(s);
		JsonParseStatus status = json_parse(source, &endptr, &value, allocator);
		if (status != JSON_PARSE_OK)
		{
			print_error("test-suite", status, endptr, source, length);
		}
		else
		{
			++passed;
		}
		free(source);
	}
	fprintf(stderr, "%zd/%zd\n", passed, sizeof(SUITE) / sizeof(SUITE[0]));

	return 0;
}
