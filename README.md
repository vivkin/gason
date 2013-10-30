# gason

gason is new version of [vjson](https://code.google.com/p/vjson) parser. It's still very **fast** and have very **simple interface**. Completly new api, different internal representation and using new C++ standard features explains why parser get a new name.

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

gason is **destructive** parser, i.e. you **source buffer** will be **modified**! Strings stores as pointers in source buffer, where terminating `"` (or any other symbol, if string have escape sequences) replaced with `'\0'`. Arrays and objects represents as linked list and not support random access

## Features

* No dependencies
* Small codebase (~450 loc)
* Small memory footprint (16-24B per value)

## Installation

1. Download latest [gason.h](https://raw.github.com/vivkin/gason/master/gason.h) and [gason.cpp](https://raw.github.com/vivkin/gason/master/gason.cpp)
2. Compile with C++11 support (`-std=c++11` flag for gcc/clang)
3. ...
4. PROFIT!

## Usage

copy-paste-and-pray!

## License

Distributed under the MIT license. Copyright (c) 2013, Ivan Vashchaev
