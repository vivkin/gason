#pragma once

#include "gason.h"

// Returns the number of characters that are written to the buffer (including the null character)
// You can call this function with buffer=nullptr to query the memory you need to allocate
// If your buffer is not big enough, the function writes until runs out of space,
//		and returns the number of characters that would have been written if the buffer was big enough
size_t jsonWrite(const JsonValue& val, size_t bufferSize, char* buffer, char* indentor = "\t");