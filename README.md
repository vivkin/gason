# gason
gason is new version of [vjson](https://code.google.com/p/vjson) parser. It's still very **fast** and have very **simple interface**. Completly new api, different internal representation and using new C++ standard features explains why parser get a new name.

- [Features](#features)
- [Installation](#installation)
- [Usage](#usage)
	- [Parsing](#parsing)
	- [Iteration](#iteration)
- [Notes](#notes)
	- [NaN-boxing](#nan-boxing)
	- [Memory management](#memory-management)
	- [Parser internals](#parser-internals)
- [Performance](#performance)
- [License](#license)

## Features
* No dependencies
* Small codebase (~450 loc)
* Small memory footprint (16-24B per value)

gason is **not strict** parser:
* Source buffer can contain more than one value (first will be parsed; pointer to the rest returns)
* Single number, string or identifier will be succesfully parsed
* Trailing `,` before closing `]` or `}` is not an error
* Control characters in strings are allowed

gason is **destructive** parser, i.e. you **source buffer** will be **modified**! Strings stored as pointers to source buffer, where closing `"` (or any other symbol, if string have escape sequences) replaced with `'\0'`. Arrays and objects are represented as single linked list (without random access).

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
JsonParseStatus status = jsonParse(source, &endptr, &value, allocator);
if (status != JSON_PARSE_OK) {
	fprintf(stderr, "error at %zd, status: %d\n", endptr - source, (int)status);
	exit(EXIT_FAILURE);
}
```
All **values** will become **invalid** when **allocator** be **destroyed**. For print verbose error message see `print_error` function in [pretty-print.cpp](pretty-print.cpp).

### Iteration
```cpp
double sum_and_print(JsonValue o) {
	double sum = 0;
	switch (o.getTag()) {
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
		for (auto i : o) {
			sum += sum_and_print(i->value);
		}
		break;
	case JSON_TAG_OBJECT:
		for (auto i : o) {
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

## Notes
### NaN-boxing
gason stores values using NaN-boxing technique. By [IEEE-754](http://en.wikipedia.org/wiki/IEEE_floating_point) standard we have 2^52-1 variants for encoding double's [NaN](http://en.wikipedia.org/wiki/NaN). So let's use this to store value type and payload:
```
 sign
 |  exponent
 |  |
[0][11111111111][yyyyxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx]
                 |   |
                 tag |
                     payload
```
48 bits payload [enough](http://en.wikipedia.org/wiki/X86-64#Virtual_address_space_details) for store any pointer on x64.

### Memory management
JsonAllocator allocates big blocks of memory and use pointer bumping inside theese blocks for smaller allocations. Size of block can be tuned by *JSON_ZONE_SIZE* constant (default 4 KiB).

### Parser internals
> [05.11.13, 2:52:33] Олег Литвин: о нихуя там свитч кейс на стеройдах!

Internally in `json_parse` function nested arrays/objects stored in array of circulary linked list of `JsonNode`. Size of that array can be tuned by *JSON_STACK_SIZE* constant (default 32).

## Performance

For build parser shootout:

1. `clone-enemy-parser.sh` (need mercurial, git, curl, nodejs)
2. `cmake -DCMAKE_BUILD_TYPE=Release -DSHOOTOUT=ON`

Test files downloads from random repos on github:
* `big.json` - just big with big count escape sequences
* `monster.json` - 3d model, lot of numbers
* `data.json` - many objects

First column - parse time in microseconds, second - traverse and sum all numbers.

Intel Core i7 2.3 GHz, OSX 10.9, clang-500.2.79:
```
shootout/big.json: length 6072200
     gason      20092us         71us 	(5520251617769.000000)
     vjson      19723us        156us 	(5520251617769.000000)
    sajson      25226us        128us 	(5520251617769.000000)
 rapidjson      18093us         96us 	(5520251617769.000000)
shootout/data.json: length 17333
     gason         75us          4us 	(3754.333493)
     vjson         80us          5us 	(3754.333471)
    sajson        117us          8us 	(3754.333493)
 rapidjson         91us          7us 	(3754.333493)
shootout/monster.json: length 196473
     gason        924us        162us 	(34474757.667613)
     vjson       2218us        396us 	(34474757.667621)
    sajson       1898us        380us 	(34474757.667613)
 rapidjson       2210us        446us 	(34474757.667613)
```
Samsung Galaxy Note II (GT-N7100), Android 4.1.1, gcc 4.8.2:
```
I/ruberoid(28228): /sdcard/Download/shootout/big.json: length 6072200
I/ruberoid(28228):      gason 5520251617769.000000      67839us        *569us*
I/ruberoid(28228):      vjson 5520251617769.000000      *61227us*       1013us
I/ruberoid(28228):     sajson 5520251617769.000000      80071us       1818us
I/ruberoid(28228):  stix-json 5520251617769.000000     100503us     185185us
I/ruberoid(28228): /sdcard/Download/shootout/data.json: length 17333
I/ruberoid(28228):      gason 3754.333493        *326us*         *31us*
I/ruberoid(28228):      vjson 3754.333471        338us         53us
I/ruberoid(28228):     sajson 3754.333493        333us         57us
I/ruberoid(28228):  stix-json 3754.333493        517us        336us
I/ruberoid(28228): /sdcard/Download/shootout/monster.json: length 196473
I/ruberoid(28228):      gason 34474757.667613       7683us       *1331us*
I/ruberoid(28228):      vjson 34474757.667621      10381us       2205us
I/ruberoid(28228):     sajson 34474757.667613       *6742us*       2127us
I/ruberoid(28228):  stix-json 34474757.667613      12546us    1172294us
```

## License
Distributed under the MIT license. Copyright (c) 2013-2014, Ivan Vashchaev
