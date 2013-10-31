# gason

gason is new version of [vjson](https://code.google.com/p/vjson) parser. It's still very **fast** and have very **simple interface**. Completly new api, different internal representation and using new C++ standard features explains why parser get a new name.

## Features

* No dependencies
* Small codebase (~450 loc)
* Small memory footprint (16-24B per value)

gason store values using NaN-boxing technique. By [IEEE-754](http://en.wikipedia.org/wiki/IEEE_floating_point) standard we have 2^52-1 variants for encoding double's [NaN](http://en.wikipedia.org/wiki/NaN). So let's use this for store value type and payload:
```
 sign
 |  exponent
 |  |
[0][11111111111][yyyyxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx]
                 |   |
                 tag |
                     payload
```
48 bits payload is enough for store any pointer on [x64](http://en.wikipedia.org/wiki/X86-64#Virtual_address_space_details).

gason is **destructive** parser, i.e. you **source buffer** will be **modified**! Strings stores as pointers in source buffer, where terminating `"` (or any other symbol, if string have escape sequences) replaced with `'\0'`. Arrays and objects represents as linked list and not support random access.

## Installation

1. Download latest [gason.h](https://raw.github.com/vivkin/gason/master/gason.h) and [gason.cpp](https://raw.github.com/vivkin/gason/master/gason.cpp)
2. Compile with C++11 support (`-std=c++11` flag for gcc/clang)
3. ...
4. PROFIT!

## Usage

### Parsing
```cpp
#include "gason.h"
...
char *source = get_useless_facebook_response(); // or read file, whatever
// do not forget terminate source string with 0
char *endptr;
JsonValue value;
JsonAllocator allocator;
JsonParseStatus status = json_parse(source, &endptr, &value, allocator);
if (status != JSON_PARSE_OK)
{
	fprintf(stderr, "error at %zd, status: %d\n", endptr - source, (int)status);
	exit(EXIT_FAILURE);
}
```
All **values** will become **invalid** while **allocator** will be **destroyed**. For print verbose error message see `print_error` function in [pretty-print.cpp](pretty-print.cpp).

### Iteration
```cpp
double sum_and_print(JsonValue o)
{
	double sum = 0;
	switch (o.getTag())
	{
		case JSON_TAG_NUMBER:
			printf("%g\n", o.toNumber());
			sum += o.toNumber();
			break;
		case JSON_TAG_BOOL:
			printf("%s\n", o.toBool() ? "true" : "false");
			break;
		case JSON_TAG_STRING:
			printf("\"%s\"\n", o.toString());
			break;
		case JSON_TAG_ARRAY:
			for (auto i : o)
			{
				sum += sum_and_print(i->value);
			}
			break;
		case JSON_TAG_OBJECT:
			for (auto i : o)
			{
				printf("%s = ", i->key);
				sum += sum_and_print(i->value);
			}
			break;
		case JSON_TAG_NULL:
			printf("null\n");
			break;
	}
	return sum;
}
...
double sum = sum_and_print(value);
printf("sum of all numbers: %g\n", sum);
```
Arrays and Objects use the same `JsonNode` struct, but for arrays valid only `next` and `value` fields!

## License

Distributed under the MIT license. Copyright (c) 2013, Ivan Vashchaev
